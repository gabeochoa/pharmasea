---
project_name: 'pub_panic'
user_name: 'choicehoney'
date: '2025-12-23'
sections_completed: ['technology_stack', 'critical_implementation_rules', 'ecs_patterns', 'build_and_test_rules', 'coding_style']
---

# Project Context: Pub Panic! (Formerly Pharmasea)

_This file contains critical rules and patterns that AI agents must follow when implementing game code in this project. Focus on unobvious details that agents might otherwise miss._

---

## Technology Stack & Versions

- **Engine**: Raylib 4.5 (Custom C++ integration)
- **Language**: C++20 (`-std=c++2a`)
- **ECS Framework**: `afterhours` (Custom ECS)
- **Networking**: Steam GameNetworkingSockets
- **Serialization**: `bitsery`
- **Utilities**: `fmt`, `magic_enum`, `tl::expected`, `nlohmann/json`
- **Build System**: Makefile / Clang++

## Critical Implementation Rules

- **Git Commits**: Use prefixes: `bf -` (bug fix), `be -` (engine/refactor), `ui -` (UI). All lowercase, no ampersands.
- **Early Returns**: Favor early returns to reduce nesting levels.
- **No Comments**: Do not add comments unless explicitly requested.
- **No `auto`**: Avoid `auto` for non-template types; use explicit types.
- **Includes**: Standard library first, then third-party, then project headers. Use `#pragma once`.
- **Logging**: Use `log_info()`, `log_warn()`, and `log_error()`. Remove verbose logs before committing.

## ECS Patterns

- **Components**:
  - Inherit from `BaseComponent`.
  - Use `struct` for components; they should be POD-like data containers.
  - Prefer tag components (no members) for state over boolean flags.
- **Systems**:
  - Inherit from `System<Components...>`.
  - Use `virtual void for_each_with(Entity&, Components&..., float dt) override`.
  - Systems must be focused and single-purpose.
- **Queries**:
  - Use `EntityQuery` fluent API: `EntityQuery().whereHasComponent<T>().gen()`.
  - Avoid manual loops over entities.
- **Singletons**:
  - Use `SINGLETON_FWD(Type)` and `SINGLETON(Type)` macros.
  - Access via `EntityHelper::get_singleton<Type>()`.

## Build & Test Rules

- **Build**: Use `make` to build `pharmasea.exe`.
- **Testing**:
  - All tests must pass in **HEADLESS** and **VISIBLE** modes before committing.
  - Run all tests: `./scripts/run_all_tests.sh` (headless) or `./scripts/run_all_tests.sh -v` (visible).
  - **No Branching in Tests**: Tests must not use `if`, `else`, or `switch`. Use assertions (`app.expect_*`) and waits (`app.wait_for_*`).
  - **No Manual System Calls**: Let the game loop process systems naturally via `app.wait_for_frames(N)`.

## Coding Style

- **Naming**: 
  - `camelCase` or `snake_case` for variables and functions.
  - `PascalCase` for Structs, Classes, and Enums.
  - `UPPER_CASE` for Constants and Macros.
- **Booleans**: Use `has_` prefix for data components (e.g., `has_health`) and `can_` for capabilities (e.g., `can_hold_item`).
- **References**: Prefer `Type &ref` over `Type *ptr`. Dereference pointers immediately when iterating.
