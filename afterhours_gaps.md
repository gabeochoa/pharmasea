# Afterhours gaps: onboarding Pub Panic! to afterhours-online

Project: Pub Panic! (`pharmasea`). This file is the pre-onboarding audit
for switching pharmasea's networking, e2e, and save paths onto the
`afterhours-online` plugin vendored in Pumpkin Beachball at
[vendor/afterhours-online](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/README.md).
It extends and corrects the earlier e2e-expansion notes in place: the
`G-E*` / `G-M*` IDs are kept, stale claims are marked, and new blockers
found by reading both codebases are added as `G-M7` and up. Nothing here
onboards or migrates code; it only records what must be true before
that work starts.

Inspected at: pharmasea `b0e4dd2c`, afterhours-online `57b54f5`
(Pumpkin Beachball consumer docs at
[docs/afterhours-online-gaps.md](/Users/gabeochoa/p/pumpkin-beachball/docs/afterhours-online-gaps.md),
library backlog at
[docs/gaps.md](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/docs/gaps.md)).
Method: read pharmasea's current networking
([src/network/server.h](/Users/gabeochoa/p/pharmasea/src/network/server.h),
[src/network/server.cpp](/Users/gabeochoa/p/pharmasea/src/network/server.cpp),
[src/network/client.cpp](/Users/gabeochoa/p/pharmasea/src/network/client.cpp),
[src/network/types.h](/Users/gabeochoa/p/pharmasea/src/network/types.h),
[src/network/internal/server.h](/Users/gabeochoa/p/pharmasea/src/network/internal/server.h),
[src/network/internal/client.cpp](/Users/gabeochoa/p/pharmasea/src/network/internal/client.cpp),
[src/network/internal/local_hub.h](/Users/gabeochoa/p/pharmasea/src/network/internal/local_hub.h)),
save/load
([src/save_game/save_game.h](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h),
[src/save_game/save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp),
[src/client_server_comm.cpp](/Users/gabeochoa/p/pharmasea/src/client_server_comm.cpp),
[src/serialization/world_snapshot_blob.cpp](/Users/gabeochoa/p/pharmasea/src/serialization/world_snapshot_blob.cpp)),
seed/state/visibility
([src/cli_args.cpp](/Users/gabeochoa/p/pharmasea/src/cli_args.cpp),
[src/map.h](/Users/gabeochoa/p/pharmasea/src/map.h),
[src/engine/statemanager.h](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h),
[src/entities/entity_helper.h](/Users/gabeochoa/p/pharmasea/src/entities/entity_helper.h)),
and e2e
([src/engine/simulated_input/e2e_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/e2e_manager.h),
[tests/e2e/run_e2e.sh](/Users/gabeochoa/p/pharmasea/tests/e2e/run_e2e.sh))
against the plugin's transport, loopback, and test helpers
([src/online/core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h),
[src/online/local.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/local.h),
[src/online/gns.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/gns.h),
[src/online/gns_impl.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/gns_impl.h),
[src/online/serialize.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/serialize.h),
[src/online/zpp_serialize.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/zpp_serialize.h),
[src/online/testing.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/testing.h))
and its reference consumer
([src/net_session.h](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.h),
[src/net_session.cpp](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.cpp),
[src/slime_network.h](/Users/gabeochoa/p/pumpkin-beachball/src/slime_network.h),
[tests/online_e2e.py](/Users/gabeochoa/p/pumpkin-beachball/tests/online_e2e.py)).

## How to read this file

Every gap carries one owner tag. Do not wait on the wrong owner.

| Tag | Meaning |
| --- | --- |
| `[PLUGIN]` | Work that only the afterhours-online library can do. Pharmasea cannot adapter around it without forking the plugin. |
| `[ADAPTER]` | Work in pharmasea's own code (session, packets, CLI, save, queries). The plugin already exposes enough, or deliberately never will. |
| `[VERIFY]` | Tests, harnesses, and evidence. No production behavior changes, but the switch must not be called done without it. |

Blocking means the networking/e2e/save switch cannot land safely
without it. Non-blocking means onboarding can proceed and the item is
tracked so it is not rediscovered.

### Onboarding gate (all blocking items)

| Gate | IDs |
| --- | --- |
| Threading + session ownership decided and built | G-M1, G-M5, G-M11 |
| Seed applied to the authoritative world before generation | G-M3 |
| Handshake, admission, and disconnect semantics mapped | G-M7, G-M10 |
| Transport/codec/channel/GNS-lifetime boundary chosen | G-M8 |
| Input and snapshot semantics preserved or explicitly changed | G-M9 |
| Host/client-symmetric player visibility for tests and UI | G-M2 |
| Save/load proven on the new path, corrupt input still safe | G-M6, G-M12 |
| Two-process CLI surface exists, then orchestration proves it | G-E7, G-M4 |
| Loopback A/B evidence old transport vs plugin transport | G-V1, G-V2 (under Verification) |

Already provided by the plugin, do not re-ask as blockers: terminal
`Disconnected` on early client failure and on capacity rejection, the
portable `DisconnectReason`, and `Transport::connection_stats()` all
landed upstream on 2026-11-05 (library gaps items 1-3, consumer
`OG-01` to `OG-03` closed). Pharmasea work for those is adapter-only
(G-M10). NAT traversal, relay, Steamworks, lobbies, and more than four
GNS peers remain future scope upstream (library gaps, Future scope)
and are not pharmasea onboarding blockers for direct-IP/LAN play.

## E2E harness

### G-E1: Typed custom-command registration (migrated) `[VERIFY]`

Status: done, kept for its two integration constraints. The hand parser
is deleted; scripts run on the upstream `E2ERunner` with game commands
registered through `testing::register_command`, polling expressed as
`wait_until`, and `--results-json` emitting the upstream schema
([e2e_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/e2e_manager.h)
`init()` / `update()`). Two constraints still bind any future harness
work, including online scripts:

- Pending commands must not live in a collection the game rebuilds.
  `Map::ensure_generated_map` can delete entities, so pharmasea ticks
  the handler pack against the singleton default collection and then
  restores the client collection (`e2e_manager.h` `update()`).
- The built-in `key` handler silently accepts unknown key names, so
  the game shadows it with a strict handler registered before the
  builtins (`register_custom_commands` in the same file). Online
  scripts must keep that ordering.

Smallest unblock: none. Re-verify both constraints after any
`E2ERunner` upgrade.

### G-E2: No wired time-scale / fast-forward for tests `[ADAPTER]` — corrected

Correction: the old note said the local parser lacks a knob the
upstream harness has. The knob exists upstream
([harness.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/e2e_testing/harness.h)
parses `--time-scale` / `--e2e-speed` into `E2EArgs`), but pharmasea
never reaches it: `process_dev_flags` in
[src/cli_args.cpp](/Users/gabeochoa/p/pharmasea/src/cli_args.cpp)
rejects unknown flags and has no time-scale entry, and
`e2e_manager.h` constructs its `E2ERunner` directly without
`harness::configure_runner`. So this is pharmasea wiring, not a missing
library feature.

- Pharma needs: one dt multiplier that speeds the simulation on both
  threads. A full day cycle is 200 s of real time, so `03` / `06` /
  `13` / `14` stay minutes-long `@slow` scripts. Scaling only the
  client frame dt desyncs e2e from the authoritative server, which
  ticks at a fixed 120 Hz in
  [src/network/server.cpp](/Users/gabeochoa/p/pharmasea/src/network/server.cpp)
  (`Server::run`, `desiredFrameRate`).
- Plugin provides: nothing time-related. `Transport` in
  [core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h)
  has no clock, and Pumpkin's session paces sends by wall clock
  precisely because e2e frame dt is not wall time
  ([src/net_session.h](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.h)
  `last_input_send` note). Expect the same interaction here.
- Smallest unblock: accept `--time-scale` in `process_dev_flags`,
  pass it through `configure_runner`, and apply the same multiplier
  to the server tick accumulator; prove `03` still passes with the
  multiplier on before using it to shorten CI.

### G-E3: No authoritative scripted player interaction `[ADAPTER]` — corrected

Correction: the old note asked the library for semantic
move/interact commands. That layer will not come from
afterhours-online: the library backlog explicitly keeps schemas,
authority, and movement in the game (library gaps items 5 and 6), and
Pumpkin's equivalent (`hold_away_from_center`-style game commands)
lives in game code. Pharmasea already owns the right shape and should
stop treating it as a library wait.

- Pharma needs: `walk_to` plus hold/release and an interact that is
  confirmed on the authoritative side. Today `walk_to`,
  `ffwd_phase`, and `save_slot` are pharmasea commands in
  [e2e_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/e2e_manager.h)
  (`register_custom_commands`), while `hold_key` / `release_key` are
  upstream builtins
  ([command_handlers.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/e2e_testing/command_handlers.h)).
  Interactions resolve through tile-in-front-of-facing checks inside
  `SystemManager::process_inputs`, so a client-predicted press and
  the server copy of the same press can pick different furniture when
  positions differ by half a tile; only the server result persists.
  That is the flake behind the serve half of
  `06_customer_serve_loop.e2e` and the choreography in
  `13_first_customer_served.e2e`.
- Plugin provides: transport only (`Transport::send` / `poll`), plus
  `FaultTransport` and sequence accounting for tests in
  [testing.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/testing.h).
  No input injection, no ECS, no interact semantics.
- Smallest unblock: keep `walk_to` game-side, but make its success
  condition read the server collection (as `expect_served` already
  does via `EQ(SystemManager::get().oldAll)` with a client fallback),
  and add one `interact` command that waits for the server-side held
  item to change instead of asserting after a fixed key tap.

### G-E6: No display-less mode for the game `[ADAPTER]` — rescoped

Rescope: this is not an afterhours-online gap and onboarding to the
plugin does not fix it. The plugin has no graphics backend at all
([core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h)
states no ECS/socket dependency; there is no display code in
`src/online/`). The blocker is pharmasea's own `App`.

- Pharma needs: a CI-runnable process with no WindowServer. Today
  `--headless` only sets `FLAG_WINDOW_HIDDEN`; `App::App` in
  [src/engine/app.cpp](/Users/gabeochoa/p/pharmasea/src/engine/app.cpp)
  still calls `InitWindow` and `LoadRenderTexture`, and `App::run`
  still loops on `WindowShouldClose` with `GetFrameTime` dt.
- Plugin provides: a listen-only endpoint shape. Pumpkin proves the
  pattern with a separate `slime-headless` dedicated binary
  (`--dedicated --headless`, no window) driving the same
  `Transport::listen` / `poll` loop; the transport never needed a
  window.
- Smallest unblock: a pharmasea `--dedicated` entry that skips `App`
  creation entirely and runs only the server tick plus transport poll
  (see G-M11). Do not route the whole game through the afterhours
  graphics layer just to get headless tests.

### G-E4: `raylib::TakeScreenshot` drops the directory on macOS `[ADAPTER]`

Unchanged and non-blocking for the online switch. The harness already
captures through `LoadImageFromTexture` + `ExportImageToMemory`
(`capture_png_to` in `e2e_manager.h`). Recorded here so the workaround
is not mistaken for plugin behavior; the fix, if ever taken, belongs
to the raylib backend, not to afterhours-online.

### G-E5: Perf assertions still have no provider hookup `[VERIFY]` — RESOLVED game-side

Resolved: `e2e_manager.h` `init()` now calls
`perf_commands::set_provider` + `register_perf_commands`. The provider
is fed from the client loop: `App::note_frame` feeds
`afterhours::FrameRateStats` (avg fps/ms) plus a 1024-frame ring for
p99. Every e2e finish logs the readout, and
`03_full_day_cycle.e2e` ends with `expect_fps_above 10` /
`expect_p99_below 200` smoke asserts (measured: 74 fps, p99 25ms at
2x). Server-tick perf stays open under the network milestone.

### G-E7: No two-process-ready binary surface `[ADAPTER]` — BLOCKING for G-M4

New. A multi-process harness cannot drive pharmasea today, independent
of any runner script.

- Pharma needs: `--host`, `--join <host>`, `--port <n>` (including
  `0` for an OS-picked ephemeral port), a `--dedicated` listen-only
  mode (G-E6), deterministic log readiness lines, and a clean exit
  code contract. Pumpkin's harness depends on exactly this shape:
  dynamic per-scenario ports, `wait_for_log` readiness instead of
  sleeps, and per-process logs
  ([tests/online_e2e.py](/Users/gabeochoa/p/pumpkin-beachball/tests/online_e2e.py)
  `ProcessHarness`).
- Plugin provides: `Transport::listen(port)` / `connect(host, port)`
  already take a port, and `LocalTransport` ignores it in-process.
  Nothing upstream is missing.
- Pharmasea today: [src/cli_args.cpp](/Users/gabeochoa/p/pharmasea/src/cli_args.cpp)
  knows `--local`, `--e2e`, `--seed`, `--load-save`, but no host/join/
  port/dedicated flags; the port is the compile-time
  `DEFAULT_PORT = 770` in
  [src/globals.h](/Users/gabeochoa/p/pharmasea/src/globals.h),
  forced again in `GnsClient::set_address`
  ([src/network/internal/client.cpp](/Users/gabeochoa/p/pharmasea/src/network/internal/client.cpp)),
  and hosting always goes through the menu role flow in
  [src/network/network.h](/Users/gabeochoa/p/pharmasea/src/network/network.h)
  (`RoleInfoMixin::set_role`).
- Smallest unblock: add the four flags, thread the port into
  `Server::start` and `Client` address setup instead of the constant,
  and print one `listening on port N` / `joined as <id>` line each.
  No transport change is needed for this step.

## Multiplayer (needed before network e2e can grow)

### G-M1: Main-thread to authoritative-server command channel `[ADAPTER]` — BLOCKING

Save is solved game-side; the general channel is still missing, and
the thread model is the real onboarding decision.

- Pharma needs: a general queued test/command channel (force phase,
  teleport, spawn, save, seed) with host authorization and an
  observable completion signal, instead of one packet type per verb.
  E2E runs on the client-main thread while save, seed, and phase live
  behind `server_only::*`, which refuses non-server threads
  ([src/client_server_comm.cpp](/Users/gabeochoa/p/pharmasea/src/client_server_comm.cpp)
  `save_game_to_slot` / `load_game_from_slot` guard on `is_server()`).
- What exists: `Server::queue_packet` feeds an
  `AtomicQueue<ClientPacket>` drained on the server thread
  (`process_incoming_packets` in
  [src/network/server.cpp](/Users/gabeochoa/p/pharmasea/src/network/server.cpp)),
  and `SaveSlot` rides it end to end
  (`Client::send_save_slot_request` ->
  `Server::process_save_slot_info` -> `server_only::save_game_to_slot`,
  verified by the `08` pair). The seed path uses the same queue
  (`Client::send_updated_seed`). Both handlers still carry
  "validate this came from the host" TODOs
  (`process_map_seed_info`, `process_save_slot_info`).
- Plugin provides: no command channel and no server thread.
  `Transport` in
  [core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h)
  is a single-threaded `poll()` event source; Pumpkin drives it from
  its game loop (`net_poll` in
  [src/net_session.cpp](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.cpp)).
  There is no queue to adopt, so pharmasea's `AtomicQueue` adapter
  stays regardless of transport.
- Smallest unblock: generalize the existing queue to one
  `TestCommand` variant with a request id, have the server publish
  completion (id + success) back through the normal packet path, and
  check host client id in the seed/save handlers. Keep the queue;
  only the verbs change.

### G-M2: Players are not queryable from the client collection `[ADAPTER]` — BLOCKING

- Pharma needs: one documented, host/client-symmetric answer to "how
  many players are in the world" (the future `expect_player_count`),
  usable by e2e, UI, and gameplay without knowing which thread or
  store holds players.
- Why it fails today: `EQ()` defaults to the calling thread's
  collection (`EntityHelper::get_current_collection()` switches on
  `is_server()` in
  [src/entities/entity_helper.h](/Users/gabeochoa/p/pharmasea/src/entities/entity_helper.h)),
  but `EntityType::Player` entities live outside the replicated
  collection on the client: the authoritative set is
  `Server::players` (`std::map<int, std::shared_ptr<Entity>>` in
  [src/network/server.h](/Users/gabeochoa/p/pharmasea/src/network/server.h)),
  and clients keep mirrors in `Client::remote_players` plus
  `Map::local_players_NOT_SERIALIZED` /
  `remote_players_NOT_SERIALIZED`
  ([src/map.h](/Users/gabeochoa/p/pharmasea/src/map.h)).
  Customers and furniture replicate through the world blob, which is
  why `expect_entity Customer` works on a client and
  `expect_entity Player` in
  [e2e_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/e2e_manager.h)
  returns 0 there. The `expect_served` handler already works around
  the same split by trying the server collection first.
- Plugin provides: deliberately nothing. The ECS layer does not exist
  (library gaps item 5); Pumpkin keeps peer-to-side maps in game code
  (`NetSession::peer_sides` in
  [src/net_session.h](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.h)).
  Waiting for a plugin `expect_player_count` is waiting on a layer
  the library has not decided to build.
- Smallest unblock: add one pharmasea helper (server: `players.size()`;
  client: `remote_players` plus the local player) and point
  `expect_entity Player`, `whereami` / `whereami_server`, and the
  lobby count UI at it. Document that `EQ().whereType(Player)` is
  server-collection-only.

### G-M3: CLI `--seed` never reaches the authoritative map `[ADAPTER]` — BLOCKING

- Pharma needs: a pre-lobby seed entry point applied when the
  authoritative world is created, so seeded e2e
  (`06 --seed e2e-serve`) pins the world it asserts on.
- Why it fails today: `--seed` in
  [src/cli_args.cpp](/Users/gabeochoa/p/pharmasea/src/cli_args.cpp)
  only sets `MAP_VIEWER_SEED` for the map viewer and `--generate-map`.
  The authoritative map is constructed with a hardcoded
  `Map("default_seed")` in the `Server` constructor
  ([src/network/server.h](/Users/gabeochoa/p/pharmasea/src/network/server.h)),
  and the only runtime seed path is the lobby `MapSeed` packet
  (`Server::process_map_seed_info` -> `Map::update_seed`, which also
  reseeds `RandomEngine`). The server thread and the client-main
  thread each own a `Map` (server `pharmacy_map`, client
  `Client::map`), so a client-side seed write cannot reach the
  authoritative one.
- Plugin provides: no session config to put the seed in.
  `Transport::listen` / `connect` take only a port/host
  ([core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h)),
  and Pumpkin's `NetSession` has no seed because its world needs no
  generation. The old note's "session config is the natural home"
  assumes the unbuilt ECS/session layer (library gaps item 5).
- Smallest unblock: thread an initial seed through
  `Server::start(port, seed)` into the `Server` constructor's `Map`,
  sourced from a new `GAME_SEED` cli value that `--seed` also sets.
  Keep the lobby packet for mid-session reseeds. No plugin change.

### G-M4: No two-process network test orchestration `[VERIFY]` — BLOCKING

- Pharma needs: a runner that spawns two game processes, picks a
  fresh port, waits on log readiness instead of sleeps, and collects
  both exit codes, for scripts `09` (host+client lobby loopback) and
  `10` (client settings isolation).
- Pharmasea today: [tests/e2e/run_e2e.sh](/Users/gabeochoa/p/pharmasea/tests/e2e/run_e2e.sh)
  runs one `--local` process per script with a fixed 900 s timeout.
  There is no port allocation, no second process, and no per-process
  log capture.
- Plugin provides: the transport half only. The 24-scenario contract
  matrix in
  [tests/online_e2e_test.cpp](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/tests/online_e2e_test.cpp)
  runs `LocalTransport` and `GnsTransport` inside one test process,
  and [docs/e2e-testing.md](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/docs/e2e-testing.md)
  explicitly assigns game-process harnesses to the consumer. Pumpkin's
  [tests/online_e2e.py](/Users/gabeochoa/p/pumpkin-beachball/tests/online_e2e.py)
  is the reference shape (spawn, readiness, dynamic ports, cleanup in
  reverse order), not a library deliverable.
- Smallest unblock: first land G-E7 (without host/join/port/dedicated
  flags no runner can start the second process), then copy the
  Pumpkin harness shape for the two scenarios. The old note's
  "nothing blocks it except G-M1/G-M2/G-M3" was incomplete: G-E7 and
  G-M7 (handshake feedback) block it too.

### G-M5: Menu/Game state singletons are shared across threads `[ADAPTER]` — BLOCKING

- Pharma needs: race-free state reads for the server and for e2e
  assertions. [src/engine/statemanager.h](/Users/gabeochoa/p/pharmasea/src/engine/statemanager.h)
  already documents the race: the server thread reads
  `MenuState` / `GameState` (for example `send_game_state_update` and
  the `in_game()` gate in `process_map_update`) while the main thread
  mutates them, under the rule "host is king in game, client is king
  in menus" served by one object.
- Plugin provides: no state model and no thread model. Pumpkin avoids
  the question by polling its transport on the game thread and keeping
  one `GameContext`; there is no server-local state copy in the plugin
  to adopt (the old note's "planned server-local state copy" is
  pharmasea's own Option B in the `statemanager.h` comment, not a
  plugin plan).
- Decision required before onboarding: either keep pharmasea's
  dedicated server thread and implement Option B (transitions go
  main-to-server through a queue; the server keeps a cached copy for
  packet emission), or collapse to the plugin's single-thread poll
  model and delete the server thread. The transport swap is small in
  both; the state ownership is the work.
- Smallest unblock: implement Option B for the two singletons only
  (queued transitions + server-cached read), since it preserves the
  existing 120 Hz authoritative loop. Record the choice here when
  made.

### G-M6: Per-customer progress is not serialized to clients `[ADAPTER]`

- Pharma needs: `num_orders_had` (drinks finished) visible on client
  mirrors, or an explicit host-only list, so a client-side "served"
  count stops reading 0.
- Evidence: `CanOrderDrink::serialize` in
  [src/components/can_order_drink.h](/Users/gabeochoa/p/pharmasea/src/components/can_order_drink.h)
  archives only `current_order` and `order_state`;
  `num_orders_had`, `num_orders_rem`, tab, tip, and bladder counters
  are outside the archive. Because the network snapshot and the save
  file share the same world blob
  ([src/map.h](/Users/gabeochoa/p/pharmasea/src/map.h)
  `Map::serialize` -> `snapshot_blob::encode_current_world`), the
  field is missing from both paths identically. E2E already works
  around it by reading the server collection on the host.
- Plugin provides: no field policy. Both plugin codecs
  ([serialize.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/serialize.h),
  [zpp_serialize.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/zpp_serialize.h))
  carry whatever fields the game archives.
- Smallest unblock: add `num_orders_had` to the one `serialize`
  function, bump the acknowledgement checksum that guards the format
  (`kExpectedComponentTypesChecksum` `static_assert` in
  [src/save_game/save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp)),
  and assert the client mirror in a two-process test (G-M4). This is
  a pharmasea one-line change, not a plugin sync feature.

### G-M7: No explicit handshake, admission, or rejection feedback `[ADAPTER]` — BLOCKING

New. Pharmasea's join works, but its failure modes are silent, which
blocks both players and tests (an e2e client cannot tell "rejected"
from "still connecting").

- Pharma needs: a Hello/Welcome-style gate that checks build
  compatibility before a player entity is created, returns a distinct
  reason on mismatch (version, full, malformed), and disconnects
  cleanly. Failure must be observable in-process, not only in a log.
- Pharmasea today: join is a `PlayerJoin` packet carrying
  `HASHED_VERSION` (`hashString(VERSION)` in
  [src/globals.h](/Users/gabeochoa/p/pharmasea/src/globals.h)).
  `Server::process_player_join_packet` in
  [src/network/server.cpp](/Users/gabeochoa/p/pharmasea/src/network/server.cpp)
  logs and returns on mismatch: no packet back, no disconnect, and
  the client id was already allocated in
  `server_process_message_string` before the check. Messages from
  unjoined connections are dropped silently. There is no capacity
  check anywhere (see G-M10).
- Plugin pattern: transport stays dumb; the game handshake carries
  `kProtocolVersion` plus `protocol_hash(label)`
  ([core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h)),
  and Pumpkin's server answers a bad Hello with `Bye` plus
  `Transport::disconnect`
  ([src/slime_network.h](/Users/gabeochoa/p/pumpkin-beachball/src/slime_network.h)
  `hello_matches`, `encode_bye`; `server_handle_packet` in
  [src/net_session.cpp](/Users/gabeochoa/p/pumpkin-beachball/src/net_session.cpp)).
  The library also guarantees a terminal `Disconnected` with a
  `DisconnectReason` when the transport itself refuses (G-M10).
- Smallest unblock: keep `PlayerJoin` as the Hello, but on version
  mismatch send an `Announcement` error (or a dedicated Bye packet)
  and drop the connection through the server's disconnect path, and
  log one structured `join_rejected reason=...` line the harness can
  wait on. No wire break for compatible clients.

### G-M8: Transport swap boundary: codec, channels, GNS lifetime, build `[ADAPTER]` — BLOCKING

New. "Switch to the plugin transport" is four separate decisions that
the old notes conflated. Only the first is the transport itself.

- Transport: easy. Pharmasea moves `std::string` buffers through
  `IServer` / `IClient` (`send_message_to_connection`,
  `send_string_to_server`); the plugin moves
  `std::span<const std::byte>` through `Transport::send` / `broadcast`
  / `poll`. An adapter that owns a `Transport` behind the existing
  internal interfaces is a contained change. Pumpkin proves a game
  can keep its own packets untouched on top
  ([src/slime_network.h](/Users/gabeochoa/p/pumpkin-beachball/src/slime_network.h)
  is game-owned; the plugin stays game-agnostic).
- Codec: do not swap with the transport. Pharmasea serializes packets,
  the world blob, and save files with default (native-endian)
  `zpp::bits` archives
  ([src/network/serialization.cpp](/Users/gabeochoa/p/pharmasea/src/network/serialization.cpp)
  `serialize_to_buffer` / `deserialize_to_packet`, which also ignore
  the decode result;
  [src/save_game/save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp)
  `save_slot`). The plugin pins little-endian through
  `WireOut` / `WireIn` in
  [zpp_serialize.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/zpp_serialize.h).
  Adopting `Wire*` changes network bytes and, if applied to the save
  path, save bytes. Smallest safe step: keep pharmasea's codec on top
  of the plugin transport (as Pumpkin initially did with its hand
  codec), add exact-length and failure checks to
  `deserialize_to_packet`, and treat any endian migration as its own
  change with golden-byte tests. Note pharmasea already vendors
  `zpp_bits` separately behind
  [src/zpp_bits_include.h](/Users/gabeochoa/p/pharmasea/src/zpp_bits_include.h);
  including the plugin's vendored copy in the same build needs one
  include point, not two.
- Channels: pharmasea drops them in one path. The base
  `internal::Server::send_message_to_connection` in
  [src/network/internal/server.h](/Users/gabeochoa/p/pharmasea/src/network/internal/server.h)
  always sends `Channel::RELIABLE` (with a TODO explaining the join
  packet was lost unreliable), `LocalServer` ignores the channel,
  while `GnsServer` and the client honor it. The plugin maps channels
  1:1 to GNS send flags and lists "no silent upgrade" as checked-not-
  a-gap (library gaps, Checked). Before switching, decide per packet
  type: the 20 Hz full-world `Map` sync in `process_map_sync` is
  marked unreliable to avoid backpressure, `PlayerLocation` is
  unreliable, join/seed/save are reliable. Preserve that table in the
  adapter and assert it in loopback tests (G-V1).
- GNS lifetime and link: pharmasea initializes GNS itself
  (`Info::init_connections` / `shutdown_connections` in
  [src/network/network.h](/Users/gabeochoa/p/pharmasea/src/network/network.h),
  `GameNetworkingSockets_Init` / `Kill` under `BUILD_WITHOUT_STEAM`,
  debug callback at Warning that `abort()`s on Bug) and links the
  dynamic `libGameNetworkingSockets.dylib` (top-level `makefile`,
  `install_name_tool` fixup). The plugin owns lifetime by refcount
  (`gns_acquire` / `gns_release` in
  [gns_impl.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/gns_impl.h))
  behind one opt-in TU
  ([gns.cpp](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/gns.cpp))
  and links the static archive from its build script. Running both
  owners double-initializes GNS. Smallest unblock: retire pharmasea's
  init/kill and dylib link in the same change that introduces
  `GnsTransport`, keep pharmasea's debug-callback policy as an
  explicit adapter choice, and check the final binary's linkage
  (`otool -L`) since the plugin's static archive still pulls dynamic
  protobuf/Abseil on this machine (library gaps item 7). Also note
  the standard mismatch: pharmasea builds `-std=c++2a` while plugin
  tests build `-std=c++23`; the headers only need C++20 (`std::span`),
  but prove it with a compile of `core.h` + `local.h` under the game
  flags before the swap.

### G-M9: Input and snapshot semantics differ from the plugin's model `[ADAPTER]` — BLOCKING

New. The plugin's reference consumer sends small, sequenced inputs
and tiny snapshots; pharmasea sends batched key snapshots and the
whole world. Onboarding must state what is preserved.

- Inputs: pharmasea's `PlayerControl` carries `UserInputs`, a vector
  of per-frame snapshots (presses, `frame_dt`, `cam_angle`) collected
  in `CollectsUserInput`
  ([src/components/collects_user_input.h](/Users/gabeochoa/p/pharmasea/src/components/collects_user_input.h)),
  sent at the client's ~25 Hz tick and cleared after send
  (`Client::send_player_input_packet` in
  [src/network/client.cpp](/Users/gabeochoa/p/pharmasea/src/network/client.cpp)).
  The server replays them through `SystemManager::process_inputs` at
  a fixed `1/120.f` dt regardless of the snapshot's recorded dt
  ([src/system/core/system_manager.cpp](/Users/gabeochoa/p/pharmasea/src/system/core/system_manager.cpp)).
  There is no tick, sequence, acknowledgement, or redundancy: a lost
  unreliable packet loses its frames. Pumpkin's `InputPacket`
  (tick, move, jump byte) at 30 Hz wall-clock with a latched jump edge
  is the contrasting shape, and the library's `SequenceTracker` in
  [testing.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/testing.h)
  exists for accounting, not for game inputs (library gaps item 6
  keeps ack/redundancy in the game).
- Snapshots: pharmasea broadcasts the full world blob (every entity,
  component mask, `kWorldSnapshotVersion = 4`, 64 MiB cap in
  [src/serialization/world_snapshot_blob.h](/Users/gabeochoa/p/pharmasea/src/serialization/world_snapshot_blob.h))
  at 20 Hz unreliable plus 100 Hz rare player data; the client
  replaces its whole collection (`decode_into_current_world`) with
  no interpolation buffer. `Map::update_map` only copies
  `showMinimap`; the world replacement happens inside the archive.
  The server file itself notes sending costs ~15x the game tick.
  Pumpkin sends a fixed-layout `MatchSnapshot` and interpolates
  ~100 ms behind; the plugin provides no snapshot buffer, sampler,
  or rate gate (library gaps item 6), so pharmasea gains none by
  switching.
- Smallest unblock: write the preserved-semantics table before coding
  (input batching kept or sequenced; snapshot stays full-blob at
  20 Hz; no interpolation in v1), then prove with the loopback matrix
  (G-V1) that loss of one input batch and one snapshot each degrade
  the same way before and after the transport swap. Any move to
  delta snapshots or input sequencing is a separate design, not part
  of onboarding.

### G-M10: Peer capacity, identity, and disconnect diagnostics `[PLUGIN]` provides, `[ADAPTER]` adopts

Partly new. The transport-level pieces pharmasea might wait for
already exist upstream; what remains is mapping pharmasea's identity
onto them.

- Plugin provides: server-allocated peer ids (`kServerPeer`, then
  `kFirstRemotePeer = 10000` up, cap 4 in `GnsTransport::Impl::kMaxPeers`),
  a terminal `Disconnected` for capacity rejection with
  `DisconnectReason::Rejected`, and `connection_stats()` (ping,
  quality, rates, pending bytes, jitter)
  ([core.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/core.h),
  [gns_impl.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/gns_impl.h)).
  `LocalTransport` reports perfect in-process stats and
  `LocalClose` / `ClosedByPeer` reasons
  ([local.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/local.h)).
- Pharmasea today: `next_client_id` also starts at 10000
  ([src/network/server.h](/Users/gabeochoa/p/pharmasea/src/network/server.h)),
  but ids are allocated on the `PlayerJoin` packet, not on transport
  connect; `MAX_CLIENTS = 4`
  ([src/globals.h](/Users/gabeochoa/p/pharmasea/src/globals.h))
  is never enforced; disconnect handling has no reason (the GNS client
  callback logs prose and sends a leave request). Pharmasea's own
  application Ping/Pong (`there_ping` / `return_ping` / `total_ping`)
  exists because no transport stats were exposed.
- Smallest unblock: enforce `MAX_CLIENTS` at join with the G-M7 Bye,
  surface `DisconnectReason` in the client's lost-connection
  announcement instead of the generic one in `Client::tick`, and keep
  the app Ping/Pong as an end-to-end check (upstream recommends the
  same) while logging `connection_stats()` in the network debug UI.
  No plugin work is blocking here; a pre-accept admission hook and
  reasons beyond the current enum remain upstream future scope
  (library gaps items 3 and 9).

### G-M11: Dedicated server topology and loop ownership `[ADAPTER]` — BLOCKING

New. Pharmasea has exactly one topology: a listen host whose process
also runs a client, started from the menu.

- Pharma needs: a listen-only server process (for tests and for a
  real dedicated host) or an explicit decision that listen-host is
  the only supported topology after onboarding.
- Pharmasea today: `RoleInfoMixin::set_role` Host starts the server
  thread and then creates a `Client` in the same process
  ([src/network/network.h](/Users/gabeochoa/p/pharmasea/src/network/network.h));
  pure-client mode is redirected to Host under `LOCAL_ONLY`. The
  server owns its loop (`Server::run`, fixed 120 Hz, plus a
  pathfinding thread). Clean exit already needs explicit
  `Server::shutdown()` + `network_info.reset()` in `main`
  ([src/game.cpp](/Users/gabeochoa/p/pharmasea/src/game.cpp));
  static destruction terminated the process (exit 134) after a
  passing run.
- Plugin shape: the endpoint is passive; the game pumps `poll()`.
  Pumpkin runs three topologies on it (listen host, client, dedicated
  headless with `--dedicated-seconds` bound, signal handling, and a
  `server-settings.json` config whose CLI flags override it). None of
  that config/ops layer is in the plugin; it is consumer code to copy
  in shape, not to import.
- Smallest unblock: decide loop ownership together with G-M5 (keep
  the server thread and give it a `Transport`, or pump on a thread
  pharmasea owns). Then add the `--dedicated` entry from G-E7 that
  runs that loop without `App`, bounded by `--dedicated-seconds` for
  tests, with `SIGINT`/`SIGTERM` stopping the transport before exit.

### G-M12: Save/load must survive the switch unchanged `[ADAPTER]` — BLOCKING

New. Save/load does not use the transport directly, but it shares the
world blob, the component checksum, and the server thread with the
network path, so a careless transport/codec change breaks saves.

- What must keep working: slot save from the authoritative map
  (`SaveGameManager::save_slot` snapshots a `Map` copy;
  [src/save_game/save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp)),
  header-only slot enumeration for the UI, the `08` roundtrip pair,
  and corrupt-slot safety (`11_corrupt_save.e2e`). Loads install into
  the server collection, run post-load fixups, and push the world to
  clients (`server_only::load_game_from_slot` in
  [src/client_server_comm.cpp](/Users/gabeochoa/p/pharmasea/src/client_server_comm.cpp)).
- Couplings to respect: (a) save bytes are default-endian `zpp::bits`
  today, so the G-M8 codec decision must explicitly exclude or
  version the save format; (b) adding serialized fields (G-M6) bumps
  `kExpectedComponentTypesChecksum` and invalidates old saves unless
  versioned; (c) the `--load-save` CLI hook lives inside
  `Server::process_map_update`, gated on `MenuState::in_game()`, and
  its file-path branch duplicates the slot branch without running
  `run_all_post_load_helpers` — unify both branches onto
  `load_game_from_slot` semantics before the switch so the new path
  inherits one loader; (d) save still requires the server thread
  (G-M1), so any G-M5/G-M11 loop change must keep a server-context
  save entry point.
- Plugin provides: nothing for persistence, by design. Pumpkin has no
  save system to copy.
- Smallest unblock: before touching transport, capture golden saves
  (one per existing slot fixture plus a fresh day-0 save), then after
  each onboarding step verify headers read, slots load, and the `08`
  pair plus `11` still pass. Treat a save that only loads on the host
  but not on a joined client as a failure (the client gets the world
  through the same blob).

## Verification (no production changes, required to call the switch done)

### G-V1: Loopback A/B transport evidence `[VERIFY]` — BLOCKING

Run the same scenarios against pharmasea's current internal
transports and the plugin's, before deleting anything. The plugin
supplies the harness pieces: the 24-scenario contract matrix with
explicit `BackendCaps` and `XFAIL` / `--strict-gaps` semantics
([tests/online_e2e_test.cpp](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/tests/online_e2e_test.cpp)),
`EventPump` predicate waits, the sequence envelope, and
`FaultTransport` (drop/duplicate/corrupt/delay above the transport)
in [testing.h](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/src/online/testing.h).
Pharmasea supplies the game scenarios: lobby join, seed apply, save
slot, full-world sync under snapshot loss, and disconnect/rejoin.
Record channel behavior (G-M8) and capacity behavior (G-M10) as
caps-table rows, not `#ifdef`s. Fault injection above the transport
does not model UDP loss under GNS retransmission; say so in results
instead of claiming loss coverage (plugin
[docs/e2e-testing.md](/Users/gabeochoa/p/pumpkin-beachball/vendor/afterhours-online/docs/e2e-testing.md)
makes the same caveat).

### G-V2: Game-process and hostile-packet evidence `[VERIFY]` — BLOCKING

After G-E7/G-M4 exist: dedicated and listen-host joins in both orders,
third/fourth client admission, fifth-peer rejection, unreachable and
unparseable targets, port already bound, graceful server exit vs kill,
client leave plus replacement, and a raw-probe matrix (wrong version,
truncated join, duplicate join, packets before join, wrong-direction
packets, input flood). Pumpkin's `tests/net_probe.cpp` plus
[tests/online_e2e.py](/Users/gabeochoa/p/pumpkin-beachball/tests/online_e2e.py)
are the reference; pharmasea has no probe today, and its current
server silently drops pre-join packets (G-M7), so several probe modes
have no observable oracle until G-M7 lands. Keep deterministic checks
green throughout: `tests/e2e/run_determinism.sh` (same seed, byte-
identical `--generate-map`) must pass before and after G-M3, since
seed handling is exactly what it guards.

### G-V3: Evidence limits to state, not hide `[VERIFY]`

Loopback on one machine proves neither LAN nor internet play. Until a
two-machine run exists, record: no second physical host, no NAT or
firewall traversal, no DNS (neither pharmasea's `host_ip_address`
field nor the plugin backend resolves names; library gaps item 4),
no IPv6 end-to-end, no Windows/Linux artifact check for the plugin's
static GNS build, and no soak beyond sequential matches. The plugin's
own evidence has the same limits (library gaps, Test evidence
limits); pharmasea inherits them by adopting it.

## Plugin/upstream work (the only asks of the library)

These are the items pharmasea cannot fix alone. None blocks a
transport-only onboarding except where noted; all are already tracked
upstream, so this section is a watch list, not new requests.

| ID | Ask | Upstream state |
| --- | --- | --- |
| G-P1 | Optional ECS/session adapter (`plugin.h` / `ecs.h`: session singleton, peer/input components, poll/send systems) or an explicit transport-only decision | Open decision, library gaps item 5 / consumer `OG-04`. Blocks nothing if pharmasea builds its own `NetSession`-shaped adapter (G-M1, G-M2, G-M11); blocks any expectation of a drop-in plugin. |
| G-P2 | Reusable snapshot buffer/sampler, adaptive interpolation delay, wall-clock rate gate, input ack/redundancy helpers | Open, library gaps item 6 / `OG-05`. Non-blocking: pharmasea's full-blob model (G-M9) does not consume them in v1. |
| G-P3 | One endpoint contract: numeric-IP-only naming or DNS resolution with explicit errors, plus bracketed IPv6 tests | Open, library gaps item 4 / `OG-06`. Non-blocking for direct-IP LAN; pharmasea must not promise hostnames (G-V3). |
| G-P4 | Pre-accept admission hook / structured rejection beyond `DisconnectReason` | Open, library gaps item 9. Non-blocking: game-level Bye (G-M7) covers v1. |
| G-P5 | Distribution: CI-built static GNS artifacts, verified fetch script, LICENSE, compatibility table | Open, library gaps item 7 / `OG-08`. Blocks release, not development onboarding; pharmasea must inspect its final binary (G-M8). |
| G-P6 | Steam bootstrap split so a future `steam.cpp` can share the connection logic | Open, library gaps item 8 / `OG-09`. Future scope for pharmasea (no Steam AppID today). |

#### G-P8: Instancing integration helpers `[PLUGIN]` — new request

Not yet tracked upstream. Pharmasea is piloting
`plugins/models.h` (`InstanceBatcher` + `draw_model_instanced`)
for furniture/character models. The core draw works; the friction
is all around it:

1. **No per-instance tint.** `draw_model_instanced` draws one
   material per group, so copies that differ only in tint
   (pharmasea: normal `WHITE` vs highlighted `GRAY`, ghost
   translucent) cannot share a batch and must keep a second
   per-copy `DrawModelEx` path. An optional per-instance color
   (attribute or group-level tint uniform set from the batcher's
   existing `key`) would collapse that to one path.
2. **No `DrawModelEx`-matching transform helper.** `model_transform`
   exists, but its Y-rotation handedness relative to raylib's
   `DrawModelEx(pos, axis=Y, angleDeg, scale)` is undocumented, so
   a consumer who needs pixel-identical placement with existing
   non-instanced draws hand-composes
   `MatrixMultiply(MatrixMultiply(MatrixScale, MatrixRotate), MatrixTranslate)`
   instead. Documenting/guaranteeing the equivalence (or offering
   `model_transform_ex` with `DrawModelEx` semantics) removes the
   guesswork.
3. **Shader ownership is hand-rolled per consumer.**
   `load_instancing_shader()` returns a bare shader; every consumer
   writes the same lazy static + loaded-flag + never-unload block.
   A batcher that optionally owns/caches the default instancing
   shader (`InstanceBatcher::flush_default()`) would delete that.
4. **No render-pass flush hook.** The batcher must be flushed
   inside the caller's 3D pass; finding that boundary (and keeping
   it correct across extra passes like minimaps) is per-game
   archaeology. An afterhours render system/pass-end callback that
   flushes a registered batcher would make adoption one line.

Also already noted in the plugin's own source: `draw_model_instanced`
cleans up instance attributes via raw `rlgl` calls (its TODO asks
to move that behind a backend call); consumers currently have to
trust that the VAO poisoning is fully undone before mixing instanced
and non-instanced draws of the same mesh in one frame, which
pharmasea does (highlighted copies). An explicit guarantee/test
would help. Non-blocking; the pilot proceeds with dual paths.

#### G-P9: E2E time-scale has no reference wiring `[PLUGIN]` — new request

Upstream `E2EArgs` parses `--time-scale`/`--e2e-speed`
(G-E2, adopted), but the value is inert: nothing in the harness
applies it, so every consumer invents host-loop surgery.
Pharmasea's version: substep the client `App::update` at real dt
(draw once) plus substep the server's fixed 120 Hz tick, both
gated to debug builds — after two failed iterations (enlarging dt
broke AI cleanup on the server and key-hold/input timing on the
client). Ask: a documented reference pattern or a small helper —
e.g. `E2ERunner` exposing a scaled-dt/step-count the host feeds
its update loop, or harness guidance that substepping (not dt
scaling) is the supported semantic. Non-blocking; pharmasea's
wiring works at 2-4x, but the next consumer will rediscover the
same two failure modes.

#### G-P10: No default font in headless raylib mode `[PLUGIN]` — new request

`RaylibHeadless::try_load_default_font()` probes for raylib's
internal `LoadFontDefault`/`isGpuReady` symbols via `dlsym`; both
are absent when raylib is dynamically linked (pharmasea links
`libraylib.5.5.0.dylib`), so headless runs have no default font.
That is load-bearing in raylib: `DrawTextEx`/`MeasureTextEx`
silently substitute `GetFontDefault()` whenever a font's texture
id is 0, so any consumer code that draws text with a
not-yet-loaded font (pharmasea: intro/loading scenes during
preload) segfaults inside raylib instead of drawing fallback
text. Ask: the headless backend should provide a usable default
font (embed a minimal font and `LoadFontFromMemory` it, or expose
an explicit `set_default_font(Font)` the consumer supplies at
init), and document that text drawing before that point is
unsupported. Game-side workaround in the meantime: skip
intro/loading text draws when headless. Non-blocking.

Related, found by the same work: raylib 5.5's `LoadFontEx` only
uploads the glyph atlas texture when a window has been created,
so headless it returns a font with `texture.id == 0` and valid
CPU-side glyphs whose images are already consumed — the atlas
cannot be regenerated from that result. Consumers must redo the
load (`LoadFileData` + `LoadFontData` + `GenImageFontAtlas` +
`LoadTextureFromImage`, the pattern in upstream
`backends/raylib/font_helper.h`) or crash in the default-font
fallback above. Worth a note in the headless backend docs, or a
small `load_font_headless_safe` helper next to `font_helper.h`.

#### G-P11: `timer::HasTimer` cannot replace serialized countdown timers `[PLUGIN]` — new request

Found evaluating `plugins/timer.h` against pharmasea's
`CooldownInfo` (embedded in several components). Field mapping
exists (`reset_to`↔`reset_time`, `remaining`↔`reset_time -
current_time`, `enabled`↔`!paused`), but three gaps block adoption:

- No serialization hook: `CooldownInfo` carries a zpp `serialize`
  (it rides inside saved/networked components); `HasTimer` has
  none, so adopting it silently drops timer state from saves.
- Opposite direction: `HasTimer` counts up toward ready;
  `CooldownInfo` counts down toward ready. Every consumer reads
  `ready()`/`pct_remaining()` in countdown terms.
- Shape: `HasTimer` is a standalone component;
  `CooldownInfo` is an embedded value struct used several-per-
  component. Adoption changes component layout everywhere.

Ask (low priority): a countdown form with a `serialize` hook, or
document `HasTimer` as UI/ephemeral-only. Pharmasea keeps
`CooldownInfo` meanwhile; `HasWork` (progress % + work callback)
is a different shape entirely and also stays local.

#### G-P12: `RngStream` sequences are not portable across STLs `[PLUGIN]` — new request

Found evaluating `src/random_engine.h` for seeded map generation.
`RngStream::set_seed(std::string)` hashes with
`std::hash<std::string>` and `get_int`/`get_float` draw through
`std::uniform_int_distribution`/`std::uniform_real_distribution`.
Both are implementation-defined: the hash differs between
libstdc++/libc++/MSVC, and the distribution algorithms are
unspecified even though `mt19937` itself is standardized. So the
same seed string yields different sequences on different
compilers/platforms — `save()`/`load()` round-trips the text, but
not the meaning. A host on one STL and a client on another would
generate different maps from the same seed.

Pharmasea's local engine (pcg32 + FNV-1a seed hash + explicit
modulo/range math) is portable bit-for-bit, which host/client
agreement and `run_determinism.sh` rely on, so `RngStream` stays
unadopted here.

Ask: make `RngStream` bit-stable — hash seed strings with a
specified algorithm (e.g. FNV-1a) and implement the int/float
draws with a specified algorithm (e.g. Lemire multiply-shift or
rejection sampling over raw engine output) instead of
`std::uniform_*_distribution`. Same ask would let the
`RandomEngine` singleton be the one seeded RNG downstream games
share. Note also the singleton is named `RandomEngine`, colliding
with the global `RandomEngine` most consumers already have; a
distinct name (`Rng`/`SeededRng`) would ease adoption.

#### G-P13: `pathfinding` plugin is not adoptable by serialized/grid movers `[PLUGIN]` — new request

Found in an API diff against pharmasea's
`astar.h`/`bfs.h`/`pathfinder.h` + `CanPathfind`. The design
matches (async request/response queue, `is_walkable` callback,
AStar/BFS), but adoption is currently a rewrite, not a swap:

- No serialization: local `CanPathfind` carries a zpp `serialize`
  (parent, start, goal, path_size) because movers are saved and
  networked; `ProvidesPathfinding` state has no hook.
- Fixed grid model: upstream paths on `Vector2Type` with
  Euclidean estimate/cost; local search snaps to the `TILESIZE`
  grid (`vec::forEachNeighbor`) and movement/collision lives in
  the component (`travel_toward`, local/global targets).
- No parity harness: there is no worked example of the plugin
  reproducing a known path set, so a consumer cannot verify
  equivalence before migrating.

Ask (low priority): a `serialize` hook for path/request state, a
pluggable neighbor/step-cost provider so grid games keep their
snapping, and a parity example (fixed map -> expected paths).
Until then pharmasea keeps its local trio; no current pain, this
only unblocks a future dedup.

#### G-P14: No allocator/arena-backed way to materialize query results `[PLUGIN]` — new request

Found evaluating `memory/arena.h` for hot queries. The arena is a
standalone bump allocator used by UI rendering; nothing connects
it to `ecs.h`. `EntityQuery::gen()` returns an owning
`RefEntities` (heap vector + shared refs) — ~102 call sites in
pharmasea run it per-frame — and while `for_each`/
`for_each_stream` already avoid materializing, consumers that
must sort, shuffle, index, or outlive the call still pay a heap
allocation per query. There is no pmr/allocator-aware `gen()`
variant and no documented frame-arena query pattern.

Ask: an allocator-aware result path (e.g. `gen_into(std::pmr::
vector<...>&)` or an arena-owning result type with explicit
lifetime), or documentation blessing `for_each` + a scratch
member vector as the intended pattern. Until upstream picks one,
trialing `Arena` locally means changing `EntityQuery` result
ownership engine-wide, which is not worth doing blind.

### Researched engine asks (G-P15-G-P24)

From a full sweep of the largest hand-rolled subsystems in `src/`
(see the evidence in each section; line counts are `wc -l` measured,
deletion figures are estimates marked *(est.)* and overlap — do not
sum naively).

## (a) Ranked top 10

| # | Feature (generic shape) | Local code it deletes (approx) | Features it unlocks | Other-games appeal (1–3) |
|

Two vendor facts shape several asks below and are worth stating once:

- The vendor UI is already large and capable — `vendor/afterhours/src/plugins/ui/` totals ~37k lines across headers measured in this session, including [imm_components.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_components.h), [setting_row.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/setting_row.h) (495 lines), [menu.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/menu.h) (468 lines), [imm_virtual_list.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_virtual_list.h) (421 lines), plus [modal.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/modal.h) (1,128 lines) and [toast.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/toast.h) (361 lines). Pharmasea calls **none** of the `imm::` API: a search for `imm::` / `ui::setup` / `ComponentConfig` / `mk(` across `src/layers`, `src/system/ui`, and `src/engine/ui` returned no hits. It instead maintains a parallel UI stack in `src/engine/ui/` (3,912 lines measured: [color.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/color.h) 979, [ui.cpp](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui.cpp) 618, [autolayout.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/autolayout.h) 515, [svg.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/svg.h) 294, [ui_input_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_input_manager.h) 224, [ui_internal.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_internal.h) 220, [focus.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/focus.h) 156, [rect.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/rect.h) 152, and smaller pieces). The asks in §1–§2 are therefore **not** "build widgets" — the widgets exist. They ask for the missing binding/scaffolding layer above them, plus a supported migration bridge so a game with an existing rect-based UI can move screen-by-screen instead of in one rewrite. That is also why the [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) "imm UI suite" skip does not settle §1: the skip declined adopting the suite wholesale; it did not evaluate a data-bound form/list layer, which does not exist upstream today.

- Pharmasea's local code totals ~65k lines under `src/` (64,967 measured, of which 12,415 are the generated `src/resources/fonts/Karmina_Regular_256.h` header — so ~52.5k hand-written lines, *(est.)* after that single exclusion). The subsystems ranked below are the largest hand-written clusters in that total.

#### G-P15: Data-bound UI scaffolding on the imm suite `[PLUGIN]` — new request

**Today.** UI is hand-laid-out immediate mode over manual rectangle carving. [rect.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/rect.h) (152 lines) provides only `lpad/rpad/tpad/bpad`, `vsplit`/`hsplit` in 2- and N-way forms — every screen then composes dozens of splits by hand. [settingslayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.cpp) (551 lines) is the clearest specimen: `draw_base_screen` splits one window into 12 rows, then repeats the same label/control block twelve times — `vsplit(row, 30)`, `text(Widget{label}, TranslatableString(...))`, then `slider`/`dropdown`/`checkbox(Widget{control}, ...)` wired field-by-field into `Settings::get()`. The widget implementations themselves live in [ui.cpp](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui.cpp) (618 lines: `button`, `checkbox`, `slider`, `dropdown`, `textfield`, focus/tab handling), backed by [autolayout.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/autolayout.h) (515), [focus.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/focus.h) (156), [ui_input_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/ui_input_manager.h) (224), and [theme.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/theme.h) (91). Screens repeat the pattern: [networklayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/networklayer.cpp) (406 lines) hand-draws a lobby player list by advancing `player_name.y += height_per` in a loop; [ui_rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ui/ui_rendering_system.cpp) (515 lines) hand-draws HUD order cards, round timers, rent bars, and bank balances with raw `raylib::DrawRectangleRounded` / `DrawCircle` calls; [menulayer.h](/Users/gabeochoa/p/pharmasea/src/layers/menulayer.h) drives the main menu through an SVG-positioned [svg.h](/Users/gabeochoa/p/pharmasea/src/engine/ui/svg.h) helper (294 lines). Across `src/layers` alone there are 182 `rect::` / `button(` / `slider(` / `dropdown(` / `checkbox(` call sites measured in this session. Localization rides along manually: every label is a `TranslatableString(strings::i18n::...)` from [strings.h](/Users/gabeochoa/p/pharmasea/src/strings.h) (522 lines), while the vendor already ships [translation.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/translation.h) (372 lines) with a compatible `TranslatableString` template that pharmasea does not use.

**What's missing upstream.** The vendor has the leaf widgets — including exactly the right leaf for settings, [setting_row.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/setting_row.h), which renders `[icon] label … [toggle|stepper|slider|dropdown]` from a `SettingRowConfig` + value pointer — and list/menu/modal primitives. What it does not have is the layer that turns a *table of bindings* into a screen: no form descriptor, no "render this vector as rows" binding beyond the [imm_virtual_list.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/imm_virtual_list.h) scrolling primitive, no settings-screen shell (title, scrollable body, footer with save/back/reset), and no bridge that lets an existing `Widget{Rectangle}`-style game adopt imm one screen at a time.

**Generic API sketch.**

```cpp
Form form{ctx, parent};
form.slider("Master Volume", data.master_volume);          // label from i18n key
form.dropdown("Language", lang_options, data.lang_index);  // options + index binding
form.toggle("Fullscreen", data.isFullscreen, [&]{ apply(); });
form.list(players, [](auto& row, Player& p){ row.text(p.name); row.icon(p.mug); });
```

**Deletes / unlocks.** Net *(est.)* ~1,100 lines: most of `src/engine/ui/` widget/layout/focus code becomes vendor code over time, and each screen sheds its split-and-wire repetition (the 12 near-identical settings blocks collapse to 12 one-liners). Unlocks: screens become cheap enough to add (a real pause menu, a recipe book, a save-slot picker are currently each a new hand-laid-out layer), focus/gamepad navigation and theme changes apply uniformly, and text measurement for localization stops being per-widget. **Other games:** every game with menus and settings re-derives this exact layer; appeal 3.

---

#### G-P16: Settings + key-remapping service `[PLUGIN]` — new request

**Today.** Persistence and remapping are three separate hand-rolled stacks. (i) Settings data: [settings.h](/Users/gabeochoa/p/pharmasea/src/engine/settings.h) (186 lines) defines `settings::Data` with a positional `zpp::bits` archive and carries the telling comment `TODO How do we support multiple versions / we dont want to add a new field and break / all past save games`; [settings.cpp](/Users/gabeochoa/p/pharmasea/src/engine/settings.cpp) (343 lines) implements load/save and a dozen `update_*_from_index` / `get_*_options` pairs. (ii) Key bindings: [keymap.h](/Users/gabeochoa/p/pharmasea/src/engine/keymap.h) (181 lines) defines `InputName` (~30 actions), `AnyInput = variant<int, GamepadAxisWithDir, GamepadButton>`, and `FullMap = map<menu::State, LayerMapping>` — bindings are per-menu-state layers; [keymap.cpp](/Users/gabeochoa/p/pharmasea/src/engine/keymap.cpp) (346 lines) plus [input_mapping_persistence.h](/Users/gabeochoa/p/pharmasea/src/input_mapping_persistence.h) (115) and [input_mapping_setup.h](/Users/gabeochoa/p/pharmasea/src/input_mapping_setup.h) (93) handle JSON round-tripping. (iii) The remap UI: the second half of [settingslayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/settingslayer.cpp) rebuilds label/icon lookups per input (`_get_label_for_key`, `_get_icon_for_gamepad`, …) and the actual capture popup is commented out — [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) records `the remap popup itself is still disabled in settingslayer.cpp`, with only the conflict check (`KeyMap::conflicts_in` → vendor `input::actions_using`) landed. The vendor already provides the two halves separately — [settings.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/settings.h) (468 lines: `ProvidesSettings<SettingsData>` with JSON/bitsery/raw load/save and `on_change` callbacks) and [input_system.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/input_system.h) (1,546 lines: polling, `name_for_input`/`icon_for_input`, gamepad mappings) — but nothing joins them into "a settings file that tolerates new fields" + "click Remap, press a key, see conflicts" that a game can drop in.

**Generic API sketch.**

```cpp
settings::store<MyData>("settings.json").version(3)
  .migrate(1, [](auto& d){ d.vsync_enabled = true; }).load();
auto r = remap_capture(ctx, parent, actions, Action::Jump); // press-any-key + conflicts
if (r) bindings.set(ctx_state, Action::Jump, *r);
```

**Deletes / unlocks.** Net *(est.)* ~800 lines across the three stacks (sharing `settingslayer.cpp` with §1 — the settings-screen rows are counted there, the store/remap machinery here). Unlocks: adding a setting stops being a save-breaking event, the disabled remap popup returns with conflict refusal built in, and keyboard/gamepad icon+label lookup (currently re-derived per screen) comes from the input plugin. **Other games:** settings + remapping is universal pre-game work; appeal 3.

---

#### G-P17: Prefab / component-bundle authoring `[PLUGIN]` — new request

**Today.** [entity_makers.cpp](/Users/gabeochoa/p/pharmasea/src/entities/entity_makers.cpp) is 1,719 lines — the largest hand-written file in `src/` — of `make_*` functions that repeat the same choreography: `make_entity` → `addComponent<Transform>` → renderer choice (`ModelRenderer` if `ENABLE_MODELS` and the model exists, else `SimpleColoredBoxRenderer`) → capability components (`CanHoldItem`, `CanBeHeld`, `IsSolid`, …) → a `HasWork().init(lambda)` for work stations. The game's own [data-driven-entity-makers.md](/Users/gabeochoa/p/pharmasea/docs/plans/data-driven-entity-makers.md) plan inventories the repetition (8 item-container functions ~120 lines, 3 ingredient-adder functions ~80, 8+ identical `HasWork` wirings) and estimates 400–500 lines saved by game-side specs alone; [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) carries the infra item `PrefabLibrary (data-driven entity builders): move repetitive makers to JSON prefabs`. A search of `vendor/afterhours/src/plugins/` for prefab/bundle facilities returned nothing — the ECS has components and queries but no bundle, archetype, or named-spawn concept.

**Generic API sketch.**

```cpp
prefabs::define("ice_machine", furniture_base, item_container{IceBag, 4},
                ingredient_adder{Ingredient::Ice}, work_station{1.5f});
Entity& e = prefabs::spawn("ice_machine", vec2{3, 5});
e.get<HasWork>().reset_speed(2.f);  // ordinary override after spawn
```

**Deletes / unlocks.** *(Est.)* ~700 lines of makers collapse into bundle definitions + a registry; the renderer-fallback and capability boilerplate is written once per bundle, not once per entity type. Unlocks: content defined in data (JSON or constexpr tables) that designers/tests/e2e can spawn by name, and a natural home for the "which components does an X have" question that save/network code also asks. The plan file's open questions (compile-time vs runtime, serialization impact) are exactly engine-level decisions — component bundles must compose with the existing `serialize` field-order lists, not bypass them. **Other games:** every ECS game writes `make_goblin()`; appeal 3.

---

#### G-P18: Save-game versioning & migration framework `[PLUGIN]` — new request

**Today.** Saves are a versioned envelope with no migration path. [save_game.h](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.h) (143 lines): `SaveGameHeader{ save_version = 1, hashed_build_version, … magic = "PHARMSAVE" }`, and `SlotInfo::state()` returns `VersionMismatch` whenever `save_version` or the build hash differs — old saves are rejected, not migrated. [save_game.cpp](/Users/gabeochoa/p/pharmasea/src/save_game/save_game.cpp) (264 lines) adds atomic write-via-temp+rename, header-only reads, and slot enumeration, guarded by `kExpectedComponentTypesChecksum` with a `static_assert` forcing a human to acknowledge any `ComponentTypes` change. The world payload is [world_snapshot_blob.cpp](/Users/gabeochoa/p/pharmasea/src/serialization/world_snapshot_blob.cpp) (295 lines): per-entity version word (`kEntitySnapshotVersion = 4`), a component-presence bitset serialized in stable `ComponentTypes` tuple order, then positional `zpp::bits` payloads — adding a field mid-tuple silently shifts every later field. Repairs live in [post_deserialize_fixups.cpp](/Users/gabeochoa/p/pharmasea/src/post_deserialize_fixups.cpp) (88) and [save_file_load_fixers.cpp](/Users/gabeochoa/p/pharmasea/src/save_file_load_fixers.cpp) (112). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) leaves `Add save versioning + migration hooks` unchecked under Phase 3, and [afterhours_gaps.md](/Users/gabeochoa/p/pharmasea/afterhours_gaps.md) G-M12 records that save, network sync, and the component checksum share one blob — so the format question cannot be solved game-locally without re-solving it for sync too.

**Generic API sketch.**

```cpp
save::envelope<MyHeader>(/*current=*/7)
  .migrate(5, [](auto& a){ a.read_old<Pos>(); a.write_new<Transform>(); })
  .migrate(6, [](auto& s){ s.rename_component("speed", "base_speed"); });
auto loaded = save::load_slot(3);  // Ok | Migrated{from=5} | Corrupt | TooNew
```

**Deletes / unlocks.** *(Est.)* ~350 lines net (the manager's classify/load plumbing plus both fixer files become registered migrations), and — more importantly — every *future* fixer stops being a new file. Unlocks: shipping content updates without wiping players' saves, and a single place where "what changed in the format" is answered for saves and network snapshots alike. The engine is the right owner because the mechanism (envelope version, ordered migration chain, too-new/corrupt discrimination, header-only preview reads) is format-agnostic; only the per-version lambdas are game code. **Other games:** any game with saves and evolving components; appeal 3.

---

#### G-P19: Attachment system (held items, furniture, hand-trucks, ropes) `[PLUGIN]` — new request

**Today.** "X holds Y" is re-implemented per pair. Components: [can_hold_item.h](/Users/gabeochoa/p/pharmasea/src/components/can_hold_item.h) (133 lines, holder↔item dual bookkeeping with `held_by`/`held_by_id` sentinels), [can_hold_furniture.h](/Users/gabeochoa/p/pharmasea/src/components/can_hold_furniture.h) (45), a `CanHoldHandTruck` component, [has_rope_to_item.h](/Users/gabeochoa/p/pharmasea/src/components/has_rope_to_item.h) (52, a `vector<EntityRef>` chain), `ConveysHeldItem`, and [custom_item_position.h](/Users/gabeochoa/p/pharmasea/src/components/custom_item_position.h) (43) for per-type held poses. Logic: grab/drop/rotate/place-validity lives in [input_process_manager.cpp](/Users/gabeochoa/p/pharmasea/src/system/input/input_process_manager.cpp) (969 lines — `handle_grab_or_drop`, `drop_held_furniture`, `work_furniture`, each re-deriving reach, walkability, and store-boundary rules, with `TODO :PICKUP: i dont like that these are spread everywhere` at the sound call sites), and held-pose solving in `src/system/sixtyfps/` (`afterhours_held_position_helpers.h` 96 + `held_position_traits.h` 53, inside a 1,258-line directory total). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) already names the consolidation: `AttachmentSystem: unify held item/furniture/handtruck/rope via generic parent→child attachment`. No attachment/parenting facility exists in the vendor plugin inventory.

**Generic API sketch.**

```cpp
attach::to(child, parent, {.socket = "hand", .filter = {Item, Furniture}});
attach::detach(child);                       // pose + velocity inherited once
if (auto held = attach::held_by<Drink>(player)) use(*held);
```

**Deletes / unlocks.** *(Est.)* ~450 lines: one relation replaces four component shapes, the per-pair grab/drop code paths, and the duplicated held-pose traits; invariants (a child has one parent; cleanup detaches) are checked once. Unlocks: new carryables (a mop buddy is already special-cased as a droppable "special boy" in `handle_drop`) stop needing new code paths, and rope/chain following falls out of nesting attachments. **Other games:** carrying, mounting, parenting, and socket attachment are near-universal; appeal 3.

---

#### G-P20: Trigger-volume + progress family `[PLUGIN]` — new request

*Distinct from G-P11:* that ask is only "give the vendor countdown timer a serialize hook." This section asks for the whole interaction pattern built on top of timers — entrant accounting, activation phases, validation, and rendering — which today is a game subsystem, not a timer.

**Today.** [is_trigger_area.h](/Users/gabeochoa/p/pharmasea/src/components/is_trigger_area.h) (264 lines) encodes a 13-type `Type` enum (lobby start, store reroll, load/save slots, planning save…), an `EntrantsRequired::{All, AllInBuilding, One}` policy, a derived `Phase::{Idle, Filling, Full, Cooldown}` from two float pairs, a `ValidationFn` callback whose result must be cached in a `mutable` field because `std::function` cannot serialize, and title/subtitle plumbing. Around it, `src/system/trigger/` totals 981 lines: [trigger_cb_on_full_progress_system.h](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_cb_on_full_progress_system.h) (361), [trigger_area_systems.cpp](/Users/gabeochoa/p/pharmasea/src/system/trigger/trigger_area_systems.cpp) (267), [update_dynamic_trigger_area_settings_system.h](/Users/gabeochoa/p/pharmasea/src/system/trigger/update_dynamic_trigger_area_settings_system.h) (188), plus entrant counters. The work/progress side is [has_work.h](/Users/gabeochoa/p/pharmasea/src/components/has_work.h) (106: `pct_work_complete` + `WorkFn` callback + progress-bar visibility flags) and [cooldown_info.h](/Users/gabeochoa/p/pharmasea/src/components/cooldown_info.h) (57), and progress *rendering* is re-derived in [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (`ProgressBarConfig`, `DrawProgressBar`, `DrawFishingGame`, ~150 lines of cube-based bars). [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) lists both infra items explicitly: `Progress/Cooldown components + ProgressRender: centralize timers/progress bars` and `TriggerAreaSystem: own entrants counting, validation, progress/cooldown and activation`.

**Generic API sketch.**

```cpp
e.addComponent<TriggerVolume>({.radius = 1.5f, .need = Entrants::AllPlayers,
                               .fill_seconds = 2.f, .validate = &can_start});
on_trigger_full<StartGame>([](Entity& zone){ begin_round(); });
progress::bar3d(entity, work.pct());  // world bar; progress::bar2d for HUD
```

**Deletes / unlocks.** *(Est.)* ~600 lines: entrant counting, phase derivation, and bar rendering move into the plugin; the game keeps its `Type` enum, validation lambdas, and on-full callbacks. Unlocks: "stand in the circle together" — the lobby/store/save interaction this game uses everywhere — becomes a declaration, and multiplayer entrant semantics (who counts, client vs server validation) are solved once. **Other games:** capture points, elevators, crafting stations, and co-op start pads are the same shape; appeal 3.

---

#### G-P21: NPC queue-interaction helper `[PLUGIN]` — new request

*Scope guard:* [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) and `fsm_todo.md` keep the M01–M24 FSM conversions game-side, and this report does not propose an engine FSM framework (see "deliberately not engine"). What is shared, and small enough to be a utility rather than a framework, is the queue behavior repeated across AI states.

**Today.** Customer AI is a 9-state machine in [is_ai_controlled.h](/Users/gabeochoa/p/pharmasea/src/components/is_ai_controlled.h) (168 lines: legal-transition table, staged `next_state`, `resume_state`, ability flags) with per-state data components (`HasAIQueueState`, `HasAIDrinkState`, `HasAIBathroomState`, …) and per-state systems under `src/system/ai/` (1,683 lines total measured for the directory's headers/cpp in the earlier pass; individual systems: bathroom 144, jukebox 126, at-register 113, clean-vomit 111, drinking 102, pay 96, queue-for-register 78, wander 66). Queue manipulation itself is centralized only halfway: [ai_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ai/ai_system.cpp) (297 lines) holds `line_add_to_queue` / `line_try_to_move_closer` / `line_leave` helpers that the register, pay, bathroom, and jukebox systems each drive with their own find-target/interact/leave choreography. The game's own [generic-ai-queue-system.md](/Users/gabeochoa/p/pharmasea/docs/plans/generic-ai-queue-system.md) plan puts four systems at ~427 lines with an identical find → join → advance → interact → leave → transition skeleton and estimates ~270 lines saved by extracting the shared utilities (its recommended Approach A).

**Generic API sketch.**

```cpp
queue::Behavior{.queue = register_queue, .at_front = &serve_customer,
                .done_state = State::Drinking};
queue::tick(npc, queue, dt);  // join, shuffle forward, fire at_front once
```

**Deletes / unlocks.** *(Est.)* ~300 lines (the plan's ~270 plus per-system target/leave glue). Unlocks: new queued behaviors (a second register, a ticket window, a bus stop) become a config + one callback, and line bugs (the day-3 "people got stuck in line" class in [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md)'s completed list) get one fix site. The state enum, transition legality, and per-state decisions stay game-side, per the FSM decision. **Other games:** any NPC that lines up; appeal 2 (common, but not universal).

---

#### G-P22: World-space labels (billboard text/icons) `[PLUGIN]` — new request

**Today.** Floating text is drawn imperatively wherever it is needed. [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (1,596 lines) alone contains `DrawFloatingText` call sites for entity IDs, debug subtypes, drink contents, uses-remaining, and AI state names (`render_debug_*` family), plus a hand-rolled `draw_billboard_from_texture_or_atlas` that probes four atlases then falls back to the texture library. Gameplay labels ride other components ad hoc: `HasSpeechBubble` gates order icons in [ui_rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ui/ui_rendering_system.cpp), `HasPatience::pct()` drives card colors, and [todo.md](/Users/gabeochoa/p/pharmasea/docs/active/todo.md) files both the bug (`Text doesnt rotate based on the camera (should billboard properly)`) and the desired shape (`BillboardText/WorldLabel component + system`). The vendor has text pieces ([curved_text.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/curved_text.h), text units/stroke under `plugins/ui/`) but no world-anchored label component with billboarding, occlusion/distance policy, and icon+text composition.

**Generic API sketch.**

```cpp
e.addComponent<WorldLabel>({.text = "2x Mojito", .icon = "mojito",
                            .billboard = true, .fade_beyond = 12.f});
labels::set_text(e, fmt::format("{}s", patience_left));  // engine draws it
```

**Deletes / unlocks.** *(Est.)* ~300 lines: the debug-label family and per-feature floating-text draws collapse into data on components, and billboarding is fixed once, in the engine, for every label. Unlocks: nameplates, speech/order bubbles, damage numbers, and tutorial prompts become component edits instead of render-system surgery. **Other games:** every 3D game floats text over things; appeal 3.

---

#### G-P23: Tile-grid procgen toolkit `[PLUGIN]` — new request

**Today.** Map generation is ~1,900 lines of game code with a generic core buried in it: [map_generation.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation.cpp) (740), [simple.h](/Users/gabeochoa/p/pharmasea/src/map_generation/simple.h) (790, under `src/map_generation/`), [wave_collapse.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/wave_collapse.cpp) (536) + header (159), [ascii_grid.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/ascii_grid.cpp) (243), [pipeline.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/pipeline.cpp) (121), [day1_required_placement.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/day1_required_placement.cpp) (145), [day1_validation.cpp](/Users/gabeochoa/p/pharmasea/src/map_generation/day1_validation.cpp) (58), and [playability_spec.h](/Users/gabeochoa/p/pharmasea/src/map_generation/playability_spec.h) (198). The generic parts — a seeded tile grid, a Wave Function Collapse solver over adjacency rules, ASCII layout stamping, and "required items placed + reachable" validation — are interleaved with bar/store building shapes. The vendor inventory has no grid/WFC/procgen plugin (pathfinding exists, G-P13 aside; it consumes walkability, it does not generate it). Seeding stays as-is per the brief (G-P12 excluded); the toolkit would accept a caller-supplied RNG stream.

**Generic API sketch.**

```cpp
auto grid = procgen::wfc(tileset, {12, 12}, rng_stream("layout"));
procgen::stamp(grid, ascii_layout, legend);
auto report = procgen::validate(grid, required({Register, 1}, {Door, 2}), reachable_from(entrance));
```

**Deletes / unlocks.** *(Est.)* ~700 lines: the WFC solver, grid, stamping, and reachability/required-placement validators become engine code with tests; pharmasea keeps its tileset, building definitions, and day-1 content rules as data. Unlocks: deterministic generation checks (the `run_determinism.sh` property — same seed, byte-identical map) become an engine-tested guarantee rather than per-game discipline. **Other games:** roguelikes, builders, and room-based games share the core, but plenty of games never generate maps; appeal 2.

---

#### G-P24: Live debug inspector `[PLUGIN]` — new request

**Today.** Debugging is a pile of one-off layers and in-world text. [gamedebuglayer.cpp](/Users/gabeochoa/p/pharmasea/src/layers/gamedebuglayer.cpp) (143) + header, [debug_settings.h](/Users/gabeochoa/p/pharmasea/src/layers/debug_settings.h) (194), [uitestlayer.h](/Users/gabeochoa/p/pharmasea/src/layers/uitestlayer.h) (83), [fpslayer.h](/Users/gabeochoa/p/pharmasea/src/layers/fpslayer.h), and the `render_debug_*` floating-text family in [rendering_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/rendering/rendering_system.cpp) (entity IDs on bounding boxes, AI state names, drink contents) each answer one question with bespoke draw code. The vendor has the ingredients — [layout_inspector.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/layout_inspector.h) (289) for UI, [debug_overlay.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/ui/debug_overlay.h), [profiling.h](/Users/gabeochoa/p/pharmasea/vendor/afterhours/src/plugins/profiling.h) (298), `system_timing.h` — but no ECS inspector: no "click an entity in the world, see/edit its components, see which systems touched it" loop.

**Generic API sketch.**

```cpp
inspector::enable(systems);            // F-key overlay, world picking built in
inspector::watch<HasPatience>("pct");  // pin a field to the HUD watch list
// click entity -> component tree -> edit field -> change applies next tick
```

**Deletes / unlocks.** *(Est.)* ~300 lines of debug layers and debug draws (replaced by registrations), and whole categories of "add a debug print, rebuild, remove it" cycles. Unlocks: designers and testers can answer "why is this customer stuck?" by clicking the customer — the question the AI debug text exists for — without a build. **Other games:** every ECS game rebuilds this; appeal 3.

---

## Deliberately not engine

Things that look generic from a distance but should stay in the game, with the reason in one line:

| Candidate | Why it stays game-side |
| --- | --- |
| Drink recipes, ingredient algebra, order validation (`validate_drink_order`, mocktail/drunk-tolerance upgrades in [ai_system.cpp](/Users/gabeochoa/p/pharmasea/src/system/ai/ai_system.cpp)) | It *is* the game's rules; no second game shares Mojito xor-counting. |
| Customer decision policy (bladder → bathroom, patience, tipping, when to leave) | Tuning and identity of this game; the engine contribution is §7's queue utility, not the decisions. |
| The explicit FSM conversions M01–M24 (game flow, day/night phase, toilet, fishing, NUX…) | Already decided in `fsm_todo.md`: transition tables are small, serialized-value-sensitive, and game-owned; only the staged-commit *pattern* (§7) is shared. |
| Bar/store economy: progression options, store stock, rent, pricing (`IsProgressionManager`, `IsBank`, `progression.h`) | Balance/content, not mechanism; an engine "shop system" would encode this game's economy. |
| Map content: building shapes, day-1 required placements, tileset art rules | Content data for §9's toolkit; the solver is generic, the bar is not. |
| Network packets, authority rules, and who-may-save/seed checks | [afterhours_gaps.md](/Users/gabeochoa/p/pharmasea/afterhours_gaps.md) already assigns schemas/authority to the game (G-M1–G-M3, G-M7); the transport stays dumb on purpose. |
| Tutorial/NUX step content and bypass automation | Ordered, hand-tuned onboarding scripts; an engine sequencer would be a worse authoring tool than the current step list. |
| Fishing minigame scoring, squirter/drink-work callbacks in `HasWork().init(...)` lambdas | Minigame rules and per-station behavior; §6 provides the progress shell, the callbacks remain the game's. |
| Intro art, logo cards, and theme assets | Already covered as an engine ask by G-P7 for the *runner*; the artwork and brand timing are the game's. |
| Sound/music selection (`SoundId`, when PICKUP vs PLACE plays) | Content and game feel; the vendor sound system already owns playback. |

## The 3 to build first

1. **§1 Data-bound UI scaffolding** — it is the largest delete (~1,100 lines *(est.)*), it unblocks §2's settings screen and every future menu/HUD, and the leaf widgets already exist upstream, so the work is the binding layer and the migration bridge, not a new widget library. Every subsequent UI the game adds gets cheaper the day this lands.
2. **§3 Prefab / component-bundle authoring** — content velocity is the game's other tax: 1,719 lines of makers for what is mostly data. Bundles also force the component-composition questions that §4 (save migrations) and §5 (attachments) need answered, so building this second de-risks both.
3. **§4 Save-game versioning & migration framework** — it is the safety gate for everything else: prefab and attachment work will add/reorder components, and today that invalidates saves (`VersionMismatch`) and trips the checksum `static_assert`. Landing migrations early means the later refactors ship without wiping players, and the same envelope serves the network snapshot that shares the blob.

§2 (settings/remap service) is the natural fourth: it rides on §1's form scaffolding, and its store half can proceed in parallel since it has no UI dependency beyond the remap widget.

*Evidence note: every path and line count above was opened or measured (`wc -l`) in this session; figures marked *(est.)* are deletion estimates derived from those counts and from the game's own plans ([data-driven-entity-makers.md](/Users/gabeochoa/p/pharmasea/docs/plans/data-driven-entity-makers.md), [generic-ai-queue-system.md](/Users/gabeochoa/p/pharmasea/docs/plans/generic-ai-queue-system.md)), not measurements of a finished migration. No file other than this one was modified.*

#### G-P7: Reusable intro cards and loading screens `[PLUGIN]` — new request

Not yet tracked upstream. Pharmasea hand-rolls the whole intro
sequence in `src/intro/`: an `IntroRunner` over ordered scenes, a
studio logo card (`LogoIntroScene`), a raylib "powered by" card
(`RaylibIntroScene`), and a loading scene driven by external
progress plus status text (`PrimaryIntroScene`). Every raylib
game rebuilds this same pipeline.

Ask: an afterhours intro plugin with (1) a scene-sequence runner
(ordered scenes, `update(dt, progress) -> done`, skip /
`finish_all`), (2) built-in logo and engine-card scenes themed by
colors, font, duration, and fade, and (3) a loading scene fed by
an external progress value and status string, with no asset
assumptions beyond a caller-supplied font/atlas. Pharmasea would
keep only its scene list and theme, and delete the local scenes.
Non-blocking for online onboarding.

Explicitly not plugin work, despite earlier wording in this file:
semantic walk/interact commands (G-E3), a general test-command queue
(G-M1), player-count queries (G-M2), seed plumbing (G-M3), state
singleton ownership (G-M5), serialized gameplay counters (G-M6), and
save/load (G-M12). Those stay in pharmasea even after onboarding.

## Game-side notes (not library asks, recorded so they are not lost)

- `log_error` aborts in debug builds (assert in the logging header),
  so e2e failure reporting must never use it; the harness reports
  with `log_warn` + stderr.
- Clean process exit required an explicit `Server::shutdown()` +
  `network_info.reset()` in `main`; static destruction of the server
  otherwise terminated the process (exit 134) after a passing run.
  Any plugin `GnsTransport` owner must be destroyed on the same
  explicit path, since the plugin deliberately leaks its callback
  registries to survive static destruction (see `gns_impl.h`).
- Lobby triggers need 2s of standing to fill progress; the bypass
  now pulses W and settles instead of one 0.5s hold, which crossed
  the area before the server copy filled it (the client filled it,
  flipped to InGame, and got reverted by the next state packet).
- The customer pay queue shares the register line with the drink
  queue, so a served customer cannot pay until everyone ahead has
  their drink; if the bar closes first they leave unpaid.
- Store stock never survived a day transition:
  `CleanUpOldStoreOptionsSystem` was registered after
  `GenerateStoreOptionsSystem` with the same run condition, so
  each morning's fresh stock was deleted in the same tick.
  Registration order swapped (cleanup first).
- Fixed: the `thread::join failed: Resource deadlock avoided`
  aborts were GameNetworkingSockets asserting on its service
  thread (lock starvation under load), pharmasea's debug callback
  calling `exit(1)` from that thread, and GNS static teardown
  then joining the same thread. The callback now `abort()`s
  instead (no static destructors), and the debug level dropped
  from Msg to Warning so the callback stops starving the GNS
  thread with per-message flushes. Re-check this policy against
  the plugin's own Warning callback in `gns_acquire` during G-M8;
  two callbacks cannot both own the process.
- Pharmasea's `LocalServer` / `LocalClient` pair and the plugin's
  `LocalTransport` are different in-process shapes (handle-based hub
  in [src/network/internal/local_hub.h](/Users/gabeochoa/p/pharmasea/src/network/internal/local_hub.h)
  vs hub-of-inboxes in the plugin). `--local` e2e exercises the
  former today; after onboarding it must exercise the latter, or the
  fast CI tier proves a transport the game no longer ships.
