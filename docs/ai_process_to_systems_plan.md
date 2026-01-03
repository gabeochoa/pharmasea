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

## Design choice: ability/component-driven vs state-specific systems

We can do either:

- **State-specific systems** per behavior/state (e.g. `AICleanVomitSystem`) — explicit and simple to reason about, but can become repetitive.
- **Ability/component-driven systems** that act on components and are free to run every frame — scales better as behaviors grow, but requires a safe way to stage/commit state transitions.

### Recommendation (best of both)

Use **ability/component-driven systems** for behavior, paired with a **two-phase state transition model**:

- Behavior systems can all run every frame and can “request” transitions without immediately mutating the authoritative `IsAIControlled::state`.
- A final “commit” system applies requested transitions once per frame, making ordering deterministic.
- A simple reset gate tag can prevent half-initialized state behavior from running.

## Proposed system breakdown

### 1) Global AI systems (cross-cutting)

These run regardless of state (or across multiple states).

- **`AIBathroomInterruptSystem`** (ability/component-driven)
  - Responsibility: detect bathroom need and request a transition.
  - Note: in the new model this should set a “next state” request, not immediately mutate `IsAIControlled::state`.

- **`AIStateResetOnEnterSystem`** (optional, later)
  - Responsibility: perform “state entry” cleanup/initialization that is currently scattered (e.g. clear targets, reset scratch components).
  - Implementation requires tracking state transitions (see “State transition tracking”).

### 2) Behavior systems (ability/component-driven)

Replace each `process_state_*` with one or more systems that operate on the relevant components/abilities.
These systems can run every frame, but they should not directly commit state changes.

Initial set (mirrors current handlers):

- `AIWanderSystem` (rate-limited; uses `HasAITargetLocation`, `HasAIWanderState`)
- `AIQueueForRegisterSystem` (rate-limited; uses `HasAITargetEntity`, `HasAIQueueState`)
- `AIAtRegisterWaitForDrinkSystem` (rate-limited; uses `HasAITargetEntity`)
- `AIDrinkingSystem` (rate-limited; uses `HasAITargetLocation`, `HasAIDrinkState`)
- `AIPaySystem` (rate-limited; uses `HasAITargetEntity`, `HasAIPayState`)
- `AIPlayJukeboxSystem` (rate-limited; uses `HasAITargetEntity`, `HasAIJukeboxState`)
- `AIBathroomSystem` (rate-limited; uses `HasAITargetEntity`, `HasAIBathroomState`)
- `AICleanVomitSystem` (rate-limited + ability-gated; uses `HasAITargetEntity`)
- `AILeaveSystem` (per-tick movement; likely not rate-limited)

### 3) State transition staging + commit

To allow all behavior systems to run every frame while keeping state transitions deterministic:

- Add `set_next_state(...)` to `IsAIControlled` (or introduce a small companion component for pending transitions).
- Behavior systems call `set_next_state(...)` (or set a transition request) instead of calling `set_state(...)`.
- A final **`AICommitNextStateSystem`** runs at the end of AI processing and:
  - applies the pending transition to `IsAIControlled::state`
  - clears the pending transition
  - sets `afterhours::tags::AINeedsResetting` so “on enter” reset logic can run

### 4) Keep “pure helpers” as helpers

Not everything should become a system. Keep these as normal functions:

- **Pure calculations / validation**: `validate_drink_order`, `get_speed_for_entity`
- **Targeting/search helpers**: `find_best_*`, `pick_random_walkable_near`, queue helpers

## Implementation strategy (to stay DRY)

### A) A shared base for behavior systems

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

### B) Ordering and safe multi-system execution

Once AI is split across many systems that can all run every frame, we must avoid accidental “half-applied transitions” or “post-transition behavior running with stale scratch state”.

In the current monolithic switch, an entity runs **exactly one state handler** per tick (after applying the global interrupt).

To preserve this with multiple systems:

- **Recommended model**: staged transitions + reset gating
  - Behavior systems compute/act and may set “next state”.
  - `AICommitNextStateSystem` applies the state change once.
  - `afterhours::tags::AINeedsResetting` prevents other systems from running until entry resets are applied.

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

