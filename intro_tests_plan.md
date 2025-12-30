# Intro Tests and Input Notes

## Status (as of 2025-12-30)

- **Implemented**
  - **Intro forcing flag exists**: `--intro` is parsed in `src/game.cpp` and forces intro behavior (`SHOW_INTRO = true; SHOW_RAYLIB_INTRO = true;`).
  - **Default skip behavior exists**: intro splash is skipped when a save file exists unless explicitly forced (see startup logic in `src/game.cpp`: `SHOW_RAYLIB_INTRO = SHOW_INTRO || !save_file_exists`).
  - **A lightweight tests framework exists** under `src/tests/` and runs at startup when `ENABLE_TESTS` is enabled (see `src/tests/all_tests.h` + `tests::run_all()` call in `src/game.cpp`).

- **Not yet implemented (this plan’s original intent)**
  - No logic-only/unit tests currently cover `LoadingProgress` or intro sequencing (`IntroScreen`) and there is no dedicated intro test harness (search in `src/tests/` shows no intro-related tests).

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
