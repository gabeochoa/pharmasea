# AI “process_*” → ECS Systems Conversion Plan

## Context / current state

Today the AI logic lives in `src/system/ai_system.cpp` as:

- A single afterhours update system (`ProcessAiSystem`) that dispatches a **state machine** via a `switch` on `IsAIControlled::state`.
- A set of internal helpers like `process_state_wander(...)`, `process_state_pay(...)`, etc.
- Per-state rate limiting is handled ad-hoc by calling `ai_tick_with_cooldown(entity, dt, interval)` inside each state handler.

This works, but it keeps AI as one “mega system” and makes it hard to scale: every new AI behavior/state increases the size and coupling of `ProcessAiSystem`.

## Goal

Convert the existing AI “process_*” style functions (especially `process_state_*`) into **first-class afterhours systems** in a way that:

- Preserves behavior (same transitions, timers, targeting, etc.)
- Scales to “a bunch more systems” without boilerplate explosion
- Keeps registration/ordering explicit and predictable

## Design choice: generic vs specific systems

We can do either:

- **Specific systems** per behavior/state (e.g. `ShouldCleanVomitSystem`) — explicit and simple to reason about, but can become repetitive.
- **Generic systems** with a small amount of template/utility glue — still explicit (one system per state), but avoids repeating the same “state filter + cooldown gate + should_run” logic everywhere.

### Recommendation (best of both)

Use **generic wrappers** to reduce boilerplate, while still registering **one system per AI state** (plus a small number of “global” AI systems). This keeps debugging straightforward (“which state system ran?”) while making it cheap to add states.

In practice this means:

- Systems are still state-named (`AiWanderSystem`, `AiPaySystem`, …) for clarity.
- They are implemented via a shared base/template that handles:
  - “is gamelike?” checks
  - “is this entity currently in the state?” filtering
  - optional ability checks
  - optional cooldown gating

## Proposed system breakdown

### 1) Global AI systems (cross-cutting)

These run regardless of state (or across multiple states).

- **`AIBathroomInterruptSystem`**
  - Responsibility: the “global interrupt” that forces Bathroom when needed (with the current exclusions).
  - Runs *before* any state system.

- **`AIStateResetOnEnterSystem`** (optional, later)
  - Responsibility: perform “state entry” cleanup/initialization that is currently scattered (e.g. clear targets, reset scratch components).
  - Implementation requires tracking state transitions (see “State transition tracking”).

### 2) One system per `IsAIControlled::State`

Replace each `process_state_*` with a system that only runs when `ctrl.state == <that state>`.

Initial set (mirrors current handlers):

- `AIWanderSystem` (rate-limited, uses `HasAITargetLocation`, `HasAIWanderState`)
- `AIQueueForRegisterSystem` (rate-limited, uses `HasAITargetEntity`, `HasAIQueueState`)
- `AIAtRegisterWaitForDrinkSystem` (rate-limited, uses `HasAITargetEntity`)
- `AIDrinkingSystem` (rate-limited, uses `HasAITargetLocation`, `HasAIDrinkState`)
- `AIPaySystem` (rate-limited, uses `HasAITargetEntity`, `HasAIPayState`)
- `AIPlayJukeboxSystem` (rate-limited, uses `HasAITargetEntity`, `HasAIJukeboxState`)
- `AIBathroomSystem` (rate-limited, uses `HasAITargetEntity`, `HasAIBathroomState`)
- `AICleanVomitSystem` (rate-limited + ability-gated, uses `HasAITargetEntity`)
- `AILeaveSystem` (likely *not* rate-limited; just path toward the exit each tick)

### 3) Keep “pure helpers” as helpers

Not everything should become a system. Keep these as normal functions:

- **Pure calculations / validation**: `validate_drink_order`, `get_speed_for_entity`
- **Targeting/search helpers**: `find_best_*`, `pick_random_walkable_near`, queue helpers

## Generic implementation strategy (to stay DRY)

### A) A shared base for state systems

Create a reusable wrapper so each new state system only defines its “step” logic:

- **State filter**: only run if `IsAIControlled::state == TargetState`
- **Optional cooldown gate**:
  - Some states should only do “heavy logic” every N seconds
  - Some should run every tick
- **Optional ability gate**:
  - e.g. `CleanVomit` should only run if the ability is present

Conceptually (pseudocode):

```cpp
template<IsAIControlled::State State, float IntervalSeconds, typename... Required>
struct AiStateSystem : afterhours::System<IsAIControlled, CanPathfind, Required...> {
  bool should_run(float) override { return GameState::get().is_game_like(); }
  void for_each_with(Entity& e, IsAIControlled& ctrl, CanPathfind&, Required&..., float dt) override {
    if (ctrl.state != State) return;
    if constexpr (IntervalSeconds > 0) {
      if (!ai_tick_with_cooldown(e, dt, IntervalSeconds)) return;
    }
    step(e, ctrl, dt);
  }
  virtual void step(Entity&, IsAIControlled&, float dt) = 0;
};
```

Then each concrete system is tiny and readable.

### B) Ordering and “only one state system per entity per frame”

Once AI is split across many systems, we must avoid accidental multi-processing when a system changes `ctrl.state`.

In the current monolithic switch, an entity runs **exactly one state handler** per tick (after applying the global interrupt).

To preserve this with multiple systems:

- **Simple option (recommended first)**: enforce ordering + early returns
  - Register `AiBathroomInterruptSystem` first
  - Register state systems next in a stable order
  - Each state system checks `ctrl.state` at the *top* and returns if it doesn’t match
  - If a state system changes state, it should not “fall through” (it naturally won’t, but later systems could run)
  - This is “mostly safe” but can still allow a second system to run if the state changes mid-tick.

- **Robust option (recommended once we add more behaviors)**: add a per-frame guard
  - Introduce a lightweight transient tag, e.g. `afterhours::tags::AIProcessedThisFrame`
  - Add a `AIClearProcessedTagSystem` that runs once per frame to clear it
  - Each AI state system sets it; all other AI state systems skip if it’s already set

This guarantees **one AI state system** runs per entity per tick even when transitions happen.

### C) State transition tracking (for “on enter” logic)

Some logic is really “on state enter” (clear targets, reset timers).

If we want to formalize that, add:

- `HasAILastState` (stores previous frame’s state)
- A lightweight transient tag, e.g. `afterhours::tags::AINeedsResetting` (set when state changes)
- `AIDetectStateChangeSystem` updates `HasAILastState` and sets `AINeedsResetting`
- `AIOnEnterResetSystem` runs “on enter” handlers for entities tagged `AINeedsResetting`, then clears the tag

This lets us:

- Move `reset_component<...>` calls out of random places
- Set cooldown intervals once on entry rather than each tick

## Cooldown handling: keep vs generalize

### Phase 1 (fast + safe)

Keep `ai_tick_with_cooldown(entity, dt, interval)` as-is and call it from state systems.
This gets us system decomposition without changing timing semantics.

### Phase 2 (more generic, less hidden mutation)

Make cooldown fully generic:

- Store desired tick interval on the entity (or per-state component)
- Have a generic `CooldownTickSystem` tick timers
- Have AI systems only *read* cooldown readiness

This makes cooldown behavior observable and avoids rewriting `reset_to` every call.

## Concrete migration steps (recommended order)

1. **Inventory & tagging**
   - Enumerate all AI “state handlers” and identify required components + tick rate.

2. **Introduce the generic base/template**
   - Add `AiStateSystem` wrapper (and optionally `AiAbilityStateSystem`).

3. **Split the monolithic `ProcessAiSystem`**
   - Replace the switch-based dispatcher with per-state systems.
   - Add `AiBathroomInterruptSystem` as its own system (runs first).

4. **Add the “one per frame” guard** (once there are 3+ state systems)
   - Prevent multi-processing when transitions occur.

5. **Optional: add state-entry system**
   - Centralize resets and initializations.

6. **Register systems**
   - Keep registration in one place (e.g. `system_manager::ai::register_ai_systems`).
   - Register in deterministic order.

## Success criteria (definition of done)

- `ai_system.cpp` no longer contains monolithic “process AI for entity” dispatch logic; it becomes a set of small systems + helpers.
- Adding a new AI state is “add a component + add a small system class + register it”.
- Cooldown behavior remains unchanged in Phase 1.
- System ordering is explicit and stable; multi-processing is prevented once we scale.

