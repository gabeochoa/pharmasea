# Project Context: 

## Critical Rules

- **ECS Patterns**
  - Use the **afterhours** library for all entity management. All queries must be performed via the fluent `EntityQuery` API (see `src/entity_query.h`).
  - The **Sophie** singleton pattern is the canonical way to access global game state (see `src/system/sophie.cpp`). Access via `Sophie::get()` – never store raw pointers to the singleton.
  - Components must be **PascalCase** structs inheriting from `BaseComponent` (see `src/components/*.h`).
  - Systems operate on `RefEntity`/`OptEntity` returned by `EntityQuery` and must not manually iterate over raw containers.

- **Coding Style**
  - Variable and function names use **snake_case** (e.g., `render_floating_name`).
  - Component and type names use **PascalCase** (e.g., `HasSubtype`, `CanBeHeld`).
  - All headers start with `#pragma once` and include guards are not used.
  - Prefer early returns for error handling; avoid deep nesting.
  - Do not use `auto` for non‑template types – write explicit types.

- **Input Handling**
  - Input is defined using the **Single Keystroke JSON** syntax. Example: `{"key":"^P"}` represents **Shift‑P**. The caret (`^`) denotes the Shift modifier.
  - **Never** encode Shift modifiers directly in code; always use the JSON representation and let the input system translate it.

- **Constraints**
  - **No `Shift` modifiers** may appear in source code – they must be expressed only via the JSON syntax.
  - Target **C++23** features: structured bindings, `std::expected`, `co_await` for async tasks, and `constexpr` lambdas are encouraged.
  - Avoid deprecated C++20 features such as `std::bind` and raw `new/delete` – use smart pointers and `std::make_unique`.
  - Do not use `std::format` (not yet available in the toolchain) – rely on `fmt` library instead.

## Implementation Patterns

- **EntityQuery Usage**
  ```cpp
  // Example: find the first Sophie entity
  auto sophie = EntityQuery()
                 .whereType(EntityType::Sophie)
                 .gen_first();
  ```
  (see `src/system/sophie.cpp` line 39)

- **Sophie Singleton**
  ```cpp
  class Sophie {
   public:
    static Sophie& get();
    // ...
  };
  // Access pattern
  Sophie::get().do_something();
  ```
  (see `src/system/sophie.cpp` line 117)

- **Component Declaration**
  ```cpp
  struct HasSubtype : public BaseComponent {
    // tag‑only component – no members
  };
  ```
  (see `src/components/has_subtype.h`)

- **System Signature**
  ```cpp
  void render_simple_normal(const Entity& entity, float dt);
  // All system functions live in `src/system/*.h` and follow the same signature.
  ```
  (see `src/system/rendering_system.h`)

- **Input JSON Example**
  ```json
  { "key": "^P" }
  ```
  This JSON is parsed by the input system and translated to a Shift‑P keystroke.

## Anti‑Patterns (What NOT to do)

- **Directly accessing Shift modifiers in code** – e.g., `if (IsKeyPressed(KEY_LEFT_SHIFT))` is prohibited.
- **Manual iteration over entity containers** – always use `EntityQuery`.
- **Using `auto` for concrete types** – write the full type name.
- **Mixing naming conventions** – do not use `camelCase` for variables or `snake_case` for components.
- **Hard‑coding singleton pointers** – never store raw `Sophie*`.
- **Using deprecated C++20 features** – avoid `std::bind`, raw `new/delete`, and `std::format`.
- **Placing system logic in UI code** – keep UI handling in the input system, not in rendering or game‑logic systems.

---
*Generated from analysis of `src/entity_query.h`, `src/system/sophie.cpp`, component headers, and project conventions in `PROJECT_RULES.md`.*
