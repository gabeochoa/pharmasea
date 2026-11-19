# Afterhours gaps found by the e2e expansion

Project: Pub Panic! (`pharmasea`). Context: hand-rolled e2e harness in
[src/engine/simulated_input/e2e_manager.h](/Users/gabeochoa/p/pharmasea/src/engine/simulated_input/e2e_manager.h)
was extended with fatal assertions before migrating to the upstream
runner. Anything below is something the game needed from the library
(or from the future multiplayer plugin) and had to work around, fake,
or defer. The multiplayer plugin in `~/p/pumpkin-beachball`
(`vendor/afterhours-online`) is the reference shape for the network
items; pharmasea is not onboarding to it yet.

## E2E harness

### G-E1: Typed custom-command registration (migrated)
Done: the hand parser is deleted. Scripts run on upstream
`E2ERunner` with game asserts/actions registered via
`register_command`, polling expressed as `wait_until`, and
`--results-json` emitting the upstream schema. Two integration
notes for the library: pending commands must not live in a
collection the game rebuilds (`Map::ensure_generated_map` calls
`delete_all_entities()`), so pharmasea ticks the handler pack
against the singleton default collection; and the built-in `key`
handler silently accepts unknown key names, so the game shadows
it with a strict one registered before the builtins.

### G-E2: No time-scale / fast-forward for tests
Upstream harness has `--time-scale` / `--e2e-speed`; the local parser
does not. A full day cycle is 200s of real time (100s day + 100s
night), so `03`/`06` are minutes-long `@slow` scripts. A dt
multiplier that speeds the simulation (not `wait_frames`, which
already counts ticks) would cut them to seconds.

### G-E3: No scripted player interaction commands
The serve half of `06_customer_serve_loop.e2e` (walk to a machine,
grab a cup, fill it, hand it to the customer) cannot be driven.
`bypass_helper` hardcodes one lobby trick (hold W). Needed from the
library: semantic move/interact commands (walk-to entity/position,
interact, hold/release) layered on `input_injector`, usable without
the full UI command stack. The game-side stopgaps
(`hold_key`/`release_key`/`walk_to` in the local harness) work but
expose how brittle raw keys are: interactions resolve through
tile-in-front-of-facing checks, and client-predicted and
server-authoritative copies of the same press can pick different
furniture when their positions differ by half a tile, with only the
server result persisting. A library walk-to/interact that steers
and confirms on the authoritative side would remove a whole class
of flake.

### G-E6: No display-less mode for the game
`--headless` hides the window (`FLAG_WINDOW_HIDDEN`) and e2e
defaults to it, so runs stop stealing focus. It still creates a
real window and GL context; a CI box with no WindowServer cannot
run it. afterhours has a NONE backend (`backends/none`,
`DisplayMode::Headless`) but pharmasea drives raylib directly from
its `App`, so adopting it means routing window creation and the
frame loop through the library graphics layer.

### G-E4: `raylib::TakeScreenshot` drops the directory on macOS
Passing `tests/e2e/name.png` writes `name.png` into the CWD. The
harness now captures via `LoadImageFromTexture` +
`ExportImageToMemory` instead. The raylib backend should honor the
full path it is given.

### G-E5: Perf assertions have no provider hookup locally
Upstream `perf_commands` (`expect_fps_above`, `expect_p99_below`)
plus `FrameRateStats` exist, but pharmasea's hand parser has no
provider registration point, so the frame-time smoke item stays
open. Resolved by G-E1's migration.

## Multiplayer (needed before network e2e can grow)

### G-M1: Main-thread -> authoritative-server command channel (save solved game-side)
E2e runs on the client-main thread. Saving a slot, setting the seed,
or forcing a phase all live behind `server_only::*`, which refuses
non-server threads. The seed path already showed the shape
(`Server::queue_packet`), and the save leg now uses it too: a
`SaveSlot` client packet queued from the main thread and handled on
the server thread (`process_save_slot_info` ->
`save_game_to_slot`), verified by the `08` pair. Still missing for
the plugin: a general queued test/command channel (force phase,
teleport, spawn) with an observable completion signal, instead of
one packet type per verb, plus host authorization on these verbs
(the handlers still carry "validate this came from the host" TODOs).

### G-M2: Players are not queryable from the client collection
`EQ().whereType(EntityType::Player)` returns 0 on the client thread:
players live in map-side lists (`local_players_NOT_SERIALIZED`,
remote mirrors), not in the replicated entity collection that `EQ`
reads, while customers and furniture do replicate. Tests need one
documented, host/client-symmetric way to ask "how many players are
in the world" (the future `expect_player_count`). Same shape as the
replay-spec note: "Use EQ() so this works in both host/client
contexts" only holds for replicated sets.

### G-M3: CLI `--seed` never reaches the authoritative map
`--seed` only feeds the map viewer and `--generate-map`; a normal
game's seed is server-owned and set through the lobby UI packet
flow. Seeded e2e (`06 --seed e2e-serve`) therefore cannot pin the
world it asserts on. Needed: a pre-lobby seed entry point the host
applies when the server starts (the plugin's session config is the
natural home).

### G-M4: No two-process network test orchestration
Scripts `09` (host+client lobby loopback) and `10` (client settings
isolation) need a runner that spawns two game processes, picks a
fresh port, waits on log readiness instead of sleeps, and collects
both exit codes. Pumpkin's `tests/online_e2e.py` is the reference.
Deferred with the network milestone; nothing game-side blocks it
except G-M1/G-M2/G-M3.

### G-M5: Menu/Game state singletons are shared across threads
`statemanager.h` already documents the race: the server thread can
read `MenuState`/`GameState` while the main thread mutates them.
E2e state assertions inherit that race (host is king in game, client
is king in menus, one object serves both). The plugin's planned
server-local state copy would give tests a clean, race-free read.

### G-M6: Per-customer progress is not serialized to clients
`CanOrderDrink::serialize` carries only `current_order` and
`order_state`; `num_orders_had` (drinks finished) never reaches
client mirrors, so a client-side "served" count always reads 0.
The e2e harness reads the server collection on the host instead.
The multiplayer plugin's state sync should either serialize these
counters or document which gameplay facts are host-only.

## Game-side notes (not library asks, recorded so they are not lost)

- `log_error` aborts in debug builds (assert in the logging header),
  so e2e failure reporting must never use it; the harness reports
  with `log_warn` + stderr.
- Clean process exit required an explicit `Server::shutdown()` +
  `network_info.reset()` in `main`; static destruction of the server
  otherwise terminated the process (exit 134) after a passing run.
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
  thread with per-message flushes.
