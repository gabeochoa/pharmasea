# PharmaSea / Pub Panic! Project Rules

These are the “house rules” for this repo. They’re intentionally short and aim
to reflect how the codebase is currently built and tested.

## Git commit format
Use short, descriptive commit messages with prefixes:
- no prefix for new features
- `bf -` for bug fixes
- `be -` for backend/engine changes or refactoring
- `ui -` for UI changes

Rules:
- Use all lowercase
- Avoid special characters like `&` (use “and”)
- Keep messages concise and descriptive

## Code style
- Keep functions focused and single-purpose
- Prefer early returns to reduce nesting
- Don’t add comments unless explicitly asked
- Avoid `auto` for non-template types; use explicit types instead
- Prefer references over pointers when possible; dereference container pointers early

## Project structure (high level)
- `src/`: main game + engine code
- `resources/`: runtime assets and config
- `vendor/`: third-party dependencies
- `output/`: build artifacts (object files, logs)

## Build system (current)
- **Primary build**: `make` (the build file is `makefile` at repo root; lowercase)
- **Output binary**: `pharmasea.exe` (even on non-Windows platforms; see `OUTPUT_EXE` in `makefile`)
- **Release packaging**: `make release` (creates `release/` folder)
- **Windows**: `WinPharmaSea/PharmaSea.sln` exists for Visual Studio builds

## Tests (current)
This repo runs its C++ tests at startup when built with tests enabled.

- **Run tests only (and exit)**:
  - `./pharmasea.exe --tests-only`
  - `./pharmasea.exe -t`

## Logging
- Use `log_info()`, `log_warn()`, `log_error()`
- Remove verbose debug logs before landing changes
