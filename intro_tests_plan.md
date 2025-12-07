# Intro Tests and Input Notes

## Plan
- Add Catch2-style logic-only tests for `LoadingProgress`/`IntroScreen` status sequencing (no raylib rendering; mock/stub intro updates).
- Add small harness (like my_name_chef `TestApp`) to stub `Settings::load_save_file` and dev flags to verify `SHOW_INTRO` skip/force logic without GPU calls.
- Keep intro skip behavior: skip when save exists unless `--intro` is passed; show intro when no save or forced.

## Input/Injection Notes
- Mirror my_name_chef approach: queue synthetic input into input maps; advance frame loop; avoid direct system calls.
- Non-blocking waits: use wait states checked each frame instead of sleeps.
- Screen/nav actions: drive via UI events (clicks/keys) rather than mutating state.
- For tests touching intro, stub rendering and focus on flag/state transitions.

## my_name_chef References
- Harness patterns live under `src/testing/` (e.g., `test_app.h`, `test_input.h`, `test_macros.h`).
- Input injection: enqueue synthetic input into input maps; advance the frame loop; avoid direct system calls.
- Waiting: non-blocking waits via wait state checked each frame, no sleeps.
- Navigation: drive screens via UI events (clicks/keys), not by mutating state directly.
- Use this style as a reference for pharmasea intro tests/harness.
