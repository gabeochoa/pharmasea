# Intro Tests and Input Notes

## Plan
- Add Catch2-style logic-only tests for `LoadingProgress`/`IntroScreen` status sequencing (no raylib rendering; mock/stub intro updates).
- Add a small harness to stub `Settings::load_save_file` and dev flags to verify `--intro` and the “skip raylib splash when a save exists” logic without GPU calls.
- Keep intro skip behavior: skip raylib splash when a save exists unless `--intro` is passed; show intro when no save or forced.

## Input/Injection Notes
- Mirror my_name_chef approach: queue synthetic input into input maps; advance frame loop; avoid direct system calls.
- Non-blocking waits: use wait states checked each frame instead of sleeps.
- Screen/nav actions: drive via UI events (clicks/keys) rather than mutating state.
- For tests touching intro, stub rendering and focus on flag/state transitions.

## Current repo notes
- Tests are run via `./pharmasea.exe --tests-only` (or `-t`) when built with tests enabled.
- Existing tests live under `src/tests/`; follow the existing patterns there for any new intro-related tests.
