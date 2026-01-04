## PharmaSea + Afterhours integration: Threading / race-condition audit

Date: 2026-01-04  
Workspace: `/workspace`  
Scope: PharmaSea game code in `src/` + the integration surface with Afterhours ECS (Afterhours library sources are not present in this workspace; this audit focuses on the call sites and assumptions inside PharmaSea).

---

## Executive summary

There are several **high-severity** threading issues that can produce **undefined behavior (UB)**, intermittent crashes, or corrupt game/network state:

- **`AtomicQueue` is not actually thread-safe** and will race/UB under real producer/consumer use (it returns references after unlocking and composes multi-step operations like `empty()` + `front()` + `pop_front()` without a single lock).
- **`GlobalValueRegister (GLOBALS)` is neither thread-safe nor lifetime-safe**: it stores raw pointers to values (including pointers to members of objects that are later destroyed) with no locking. This affects `is_server()`, server debug rendering, and multiple “server only” code paths.
- **`MenuState` / `GameState` are not synchronized but are read/written from multiple threads**, causing data races.
- **The main/render thread can read `server_map` while the server thread mutates it** (host debug rendering), which is a direct data race on a complex object graph.
- **`PathRequestManager::running` is a non-atomic flag shared across threads**, which is a data race (UB) even if it “seems to work”.

If you fix only one thing first: **replace/fix `AtomicQueue` and remove cross-thread raw access to `Map` and state singletons**. Those are the most likely sources of “heisenbugs”.

---

## Thread model (as implemented today)

### Threads observed in-repo

- **Main thread**: runs the game loop, rendering, UI, and the local client’s `network::Client::tick()` (no separate client thread in this code).
- **Server thread**: created in `network::Server::start()` and runs `network::Server::run()` / `tick()` at ~120Hz.
- **Pathfinding thread**: created inside `network::Server::run()` via `PathRequestManager::start()`. Processes path requests and produces responses.
- **Timer threads**: `src/engine/timer.h` implements `interval_timer` that spawns a `std::thread` per timer instance.

### Key “shared state” surfaces

- **Queues between threads**:
  - `network::Server::{incoming_message_queue, incoming_packet_queue, packet_queue}`
  - `PathRequestManager::{request_queue, response_queue}`
- **Global registry**: `GLOBALS` stores many pointers (server ptr, server map ptr, thread IDs, flags, etc).
- **ECS state selection**: PharmaSea’s `EntityHelper::get_current_collection()` relies on `is_server()` (which relies on `GLOBALS`).
- **Singleton state machines**: `MenuState` and `GameState` are plain, non-atomic objects.

---

## Findings (issues, impact, and fixes)

### 1) `AtomicQueue` is not thread-safe (UB) under real concurrency

**Where**
- `src/engine/atomic_queue.h`
- Used heavily by `network::Server` and `PathRequestManager`.

**What’s wrong**
- `front()` returns `T&` but releases the mutex before the caller uses the reference:
  - Another thread can `pop_front()` or mutate the underlying deque before the reference is used.
  - This is classic **use-after-unlock** leading to UB.
- Call sites often do:
  - `while (!q.empty()) { auto& x = q.front(); ...; q.pop_front(); }`
  - Even if `front()` were safe, `empty()` and `front()/pop_front()` are not atomic as a group. Another thread can interleave between them.

**Why it matters**
- This queue is the backbone of “server thread ↔ main thread” coordination and “server thread ↔ pathfinding thread”.
- Under load, it can manifest as:
  - corrupted packets/messages,
  - sporadic crashes in serialization or packet dispatch,
  - missing or duplicated work items,
  - “impossible” state (e.g., popped empty, invalid references).

**Fix options**

- **Option A (recommended): Replace API with pop-returns-value**
  - Change `AtomicQueue` to something like:
    - `bool try_pop_front(T& out)` or `std::optional<T> try_pop_front()`
    - `void push(T value)` (move-friendly)
  - Make consumption a single lock-held operation.
  - **Pros**: Correct, simple, keeps current design.
  - **Cons**: Requires updating all call sites.
  - **Effort**: **Small–Medium** (mechanical refactor, but many usages).

- **Option B: Use a known-good queue**
  - e.g. `moodycamel::ConcurrentQueue` (already vendored inside Tracy, but you probably shouldn’t depend on Tracy’s copy) or add a dedicated dependency.
  - **Pros**: Performance and correctness, mature implementation.
  - **Cons**: Adds/standardizes a third-party queue dependency, API refactor still needed.
  - **Effort**: **Medium**.

- **Option C: Blocking queue (mutex + cv)**
  - If you want to avoid busy loops and reduce CPU:
    - `push()` notifies a `condition_variable`
    - consumer `wait_pop()` blocks until work arrives or shutdown
  - **Pros**: Lower CPU, smoother scheduling.
  - **Cons**: Slightly more complex shutdown semantics.
  - **Effort**: **Medium**.

---

### 2) `GLOBALS` is unsafe: data races + dangling pointers

**Where**
- `src/engine/globals_register.h` (`GlobalValueRegister`)
- Used throughout (`GLOBALS.set`, `GLOBALS.get_ptr`, `GLOBALS.get_or_default`, etc).

**What’s wrong**
- The registry stores `void*` with no ownership/lifetime tracking.
  - Example: `GLOBALS.set("server_thread_id", &server_thread_id);` where `server_thread_id` is a **member** of `network::Info`.
  - Example: `GLOBALS.set("server_thread_id", &thread_id);` where `thread_id` is a **member** of `network::Server`.
  - When those objects are destroyed, the pointer in `GLOBALS` becomes dangling; later reads are UB.
- The registry has **no mutex**:
  - concurrent reads/writes (`set`, `get_ptr`, `contains`, etc.) are data races.
- It is used to gate thread-specific behavior (`is_server()`), so a dangling `server_thread_id` can silently route logic to the wrong “side” (client vs server collection), or crash.

**Why it matters**
- This impacts multiple critical operations:
  - `is_server()` (`src/engine/is_server.h`) relies on `GLOBALS.get_or_default("server_thread_id", ...)` dereferencing an unsafe pointer.
  - Rendering can read `server_map` via `GLOBALS` while the server thread mutates it (see Finding #4).
  - “Server only” helpers fetch `GLOBALS.get_ptr<network::Server>("server")` and call into server data structures without synchronization.

**Fix options**

- **Option A (recommended): Replace pointer registry with a typed, owned snapshot struct**
  - Create a single `struct RuntimeContext { ... }` owned by the application and passed explicitly (or via a thread-safe singleton).
  - Store **values**, not pointers (e.g., `std::thread::id server_thread_id;`).
  - Use atomics for simple flags/IDs, and locks for compound objects.
  - **Pros**: Eliminates dangling pointers and makes thread boundaries explicit.
  - **Cons**: Touches many call sites and some architecture.
  - **Effort**: **Large** (but very high payoff).

- **Option B: Make `GLOBALS` minimally safe**
  - Add a mutex around its map and operations.
  - Add `unset(name)` and ensure lifecycle cleanup on shutdown.
  - Prohibit storing pointers to non-static storage (enforce via code review / helper APIs).
  - **Pros**: Faster to implement; reduces UB.
  - **Cons**: Still easy to misuse; still pointer/lifetime hazards unless strictly enforced.
  - **Effort**: **Medium**.

- **Option C: Replace with `std::any`/`std::variant` values**
  - Store actual values inside the registry (e.g., `std::any`), not pointers.
  - **Pros**: Fixes lifetime; easier migration than a full context refactor.
  - **Cons**: Type-safety is still weak; `any_cast` failures are runtime.
  - **Effort**: **Medium–Large**.

---

### 3) `MenuState` / `GameState` are accessed cross-thread without synchronization

**Where**
- `src/engine/statemanager.h` (`StateManager2`, `MenuState`, `GameState`)
- Read on server thread: `network::Server::send_game_state_update()` reads both.
- Written on main thread: UI layers and client code (`network::Info::reset_connections`, `Client::client_process_message_string`, input handlers).

**What’s wrong**
- `StateManager2` stores `state` and a `std::stack` without any lock or atomic.
- Concurrent reads/writes are a **data race** (UB), not “just stale reads”.

**Why it matters**
- This can manifest as:
  - server broadcasting inconsistent state,
  - transitions being skipped or duplicated,
  - stack corruption (prev history) leading to crashes.

**Fix options**

- **Option A (recommended): Make state thread-owned**
  - Decide one thread “owns” state transitions (usually the main thread), and the server thread receives state changes through a message queue.
  - **Pros**: Cleanest; avoids locks; aligns with deterministic game-loop design.
  - **Cons**: Requires an explicit messaging path.
  - **Effort**: **Medium**.

- **Option B: Add locking to `StateManager2`**
  - Guard `state`, `prev`, and callbacks with a mutex.
  - **Pros**: Localized change.
  - **Cons**: Easy to deadlock if callbacks call back into state; adds contention; still doesn’t solve broader shared-data issues.
  - **Effort**: **Small–Medium**.

- **Option C: Make only `state` atomic (partial fix)**
  - `std::atomic<T> state` for `read/set/is` and remove `prev` history cross-thread.
  - **Pros**: Very cheap for the common “read current state” case.
  - **Cons**: Doesn’t support safe history stack; partial behavior change.
  - **Effort**: **Small**.

---

### 4) Cross-thread access to `Map` and server internals (debug rendering and helpers)

**Where**
- `network::Server` stores `pharmacy_map` and registers it globally:
  - `GLOBALS.set("server_map", pharmacy_map.get());`
- Main thread rendering can select server map:
  - `GameLayer::draw_world()` uses `GLOBALS.get_ptr<Map>("server_map")` when `network_ui_enabled`.
- Server-only helpers fetch server pointer from `GLOBALS`:
  - `src/client_server_comm.cpp`
  - `src/system/system_manager.cpp` (uses `GLOBALS.get_ptr<network::Server>("server")`)

**What’s wrong**
- If the main thread reads/draws from `server_map` while the server thread is updating it, that is a data race on the entire world state.
- Accessing `network::Server*` from non-server thread without locks is also a data race (players map, server_p internals, etc.).

**Why it matters**
- `Map` is a large aggregate with many nested containers and pointers. Unsynchronized concurrent access can corrupt internal invariants and crash in unrelated places (e.g., rendering).

**Fix options**

- **Option A (recommended): Never read server state cross-thread; render from a snapshot**
  - Server thread periodically publishes an immutable snapshot (copy or serialized blob) to the main thread via a queue.
  - Main thread renders the snapshot only.
  - **Pros**: Strong isolation; deterministic; easy to reason about.
  - **Cons**: Copy/serialization cost; need a well-defined snapshot format.
  - **Effort**: **Medium–Large** (depending on snapshot size).

- **Option B: Shared `Map` with read/write lock**
  - Server thread takes a write lock during update; main thread takes a read lock during draw.
  - **Pros**: Minimal behavior change; debug mode becomes safe.
  - **Cons**: Lock contention can tank frame times; lock scope will be large unless carefully designed.
  - **Effort**: **Medium**.

- **Option C: Host mode runs server simulation on the main thread**
  - Remove the server thread in single-process host mode; keep pathfinding async if desired.
  - **Pros**: Eliminates biggest class of races; simpler mental model.
  - **Cons**: Host may hitch if networking/work spikes; requires reorganizing tick scheduling.
  - **Effort**: **Medium–Large**.

---

### 5) `PathRequestManager::running` is a non-atomic cross-thread flag (UB)

**Where**
- `src/engine/path_request_manager.h` / `.cpp`

**What’s wrong**
- `running` is `bool`, written by one thread (`stop()`) and read by another (`run()` loop).
- This is a C++ data race and therefore UB.

**Additional concerns**
- `stop()` does not join; it only sets the flag. Join is currently performed by whoever owns the `std::thread` returned by `start()` (today: server thread joins `pathfinding_thread` in `Server::~Server()`).
- `start()` overwrites the global `g_path_request_manager` without guarding against multiple starts.

**Fix options**

- **Option A (recommended): Use `std::atomic<bool>` for `running`**
  - Simple and correct for a stop flag.
  - **Pros**: Minimal changes.
  - **Cons**: Still busy-spins; doesn’t address queue correctness.
  - **Effort**: **Small**.

- **Option B: Switch to `std::jthread` + `stop_token`**
  - Encodes cooperative cancellation and simplifies shutdown.
  - **Pros**: Clean shutdown; avoids custom stop plumbing.
  - **Cons**: Toolchain constraints (comment in code mentions waiting on clang support).
  - **Effort**: **Medium**.

- **Option C: Combine with blocking queue**
  - Replace polling loop with `cv.wait()` to eliminate the `sleep_for` loop.
  - **Pros**: Better CPU and latency under load.
  - **Cons**: Slightly more complexity.
  - **Effort**: **Medium**.

---

### 6) `Server::queue_packet()` can dereference a null server

**Where**
- `src/network/server.cpp`:
  - `void Server::queue_packet(const ClientPacket& p) { g_server->incoming_packet_queue.push_back(p); }`

**What’s wrong**
- No `if (!g_server) return;` guard (unlike `forward_packet`).

**Why it matters**
- It turns “call ordering” bugs into hard crashes.

**Fix options**
- Add a null check (and maybe log once).
- **Effort**: **Small**.

---

## Afterhours integration notes (risk areas)

Even though Afterhours sources are not included here, PharmaSea’s integration has a few concurrency-sensitive assumptions:

- PharmaSea selects `client_collection` vs `server_collection` via `is_server()` (`src/entity_helper.h`), which depends on `GLOBALS` thread-id pointers. If `GLOBALS` is wrong/dangling, you can resolve entities out of the wrong collection.
- The code also calls `afterhours::EntityHelper::set_default_collection(...)` from:
  - `network::Server::run()` (server thread)
  - `network::Client::Client()` (main/client thread)
  - If Afterhours’ “default collection” is a process-global pointer (not thread-local), this would be fundamentally unsafe in a dual-thread (host+client) runtime. If it is thread-local, it’s fine. **Worth verifying in Afterhours.**

Recommendation: make the “current collection” selection **explicit** and/or **thread-local** in the Afterhours layer, and avoid routing through `GLOBALS`.

---

## Recommended remediation plan (pragmatic sequence)

### Phase 0: Stop the bleeding (correctness first)

- Replace `AtomicQueue` with a correct pop-by-value API (Finding #1).
- Make `PathRequestManager::running` atomic (Finding #5).
- Remove server-map debug rendering from the live server `Map`:
  - render from client map only, or from a snapshot (Finding #4).

**Expected effort**: **Small–Medium**, high crash-rate reduction.

### Phase 1: Make thread boundaries explicit

- Decide ownership for:
  - Menu/Game state transitions (main-thread-owned recommended).
  - Server authoritative simulation state (server thread).
- Add message passing for:
  - state changes (main → server),
  - debug/telemetry snapshots (server → main).

**Expected effort**: **Medium**, big long-term simplicity win.

### Phase 2: Replace or heavily constrain `GLOBALS`

- Move to a typed runtime context or a value-based registry.
- Remove storing pointers to ephemeral objects (thread IDs, stack vars, members).
- Add lifecycle cleanup/unset where needed.

**Expected effort**: **Medium–Large**, removes a whole class of UB.

---

## Tooling suggestion: catch these with sanitizers

If your build pipeline allows it, run **ThreadSanitizer** on a minimal host+client scenario (spawn host, connect local client, run for 30–60 seconds with debug UI toggles):

- **Pros**: TSAN will flag the exact data races (state, globals, map access).
- **Cons**: Slower runtime, requires compatible toolchain and build flags.
- **Effort**: **Small** to enable, **Medium** to fix resulting findings.

