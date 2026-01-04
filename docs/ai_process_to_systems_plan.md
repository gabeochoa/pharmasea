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

- **Force leave override**
  - We treat “bar is closing / end-of-round” as highest priority.
  - In `AICommitNextStateSystem`, if a “force leave” condition is active, we override any pending transition and commit `State::Leave`.

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
- As soon as a behavior system sets a next state, it should also set a lightweight transient tag so other AI behavior systems can skip for that entity until the next state is committed:
  - `afterhours::tags::AITransitionPending`
- A final **`AICommitNextStateSystem`** runs at the end of AI processing and:
  - applies the pending transition to `IsAIControlled::state`
  - clears the pending transition
  - sets `afterhours::tags::AINeedsResetting` (always) so “on enter” reset logic can run

#### Fallback-to-wander (explicit “progress or wander” policy)

We want Wander to be the universal fallback when progress is blocked.
Instead of having each behavior system directly switch to Wander, use a dedicated fallback system:

- **`AIFallbackToWanderSystem`**
  - Runs even if other AI behavior systems are skipped (but should still respect “force leave”).
  - If the entity has no pending transition (`!AITransitionPending`) and needs a fallback, it calls `set_next_state(Wander)` and sets `AITransitionPending`.
  - Should be registered early so that when it sets a transition, other systems skip for that entity.

In the current AI implementation, the states that explicitly fall back to Wander are:

- `QueueForRegister` → Wander (when no register is available)
- `Pay` → Wander (when no register is available)
- `Bathroom` → Wander (if `CanOrderDrink` is missing; safety fallback)

This list can expand as new behaviors are added.

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
  - If an entity has `afterhours::tags::AITransitionPending`, all other AI behavior systems should be registered with a `whereNotTag(AITransitionPending)`-style filter so they skip for that entity.
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

#### Note on “always set `AINeedsResetting`”

Always setting `AINeedsResetting` after a commit is the safest default, but it can introduce an extra “entry reset” step even for trivial transitions.
If that ever becomes a problem, two alternatives:

- **Selective reset tagging**: only set `AINeedsResetting` for transitions that need cleanup (e.g. ones that change target/scratch components).
- **Split reset systems**: keep `AINeedsResetting` always-on, but make `AIOnEnterResetSystem` very cheap and only do targeted resets based on (old_state, new_state).

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
   - Catalog all current `set_state(...)` transitions (from → to).
   - Catalog all current “entry/exit” cleanup:
     - `reset_component<...>`
     - `removeComponentIfExists<...>`
     - target clears (`tgt.entity.clear()`, `tgt.pos.reset()`)
   - Define initial “fallback-needed” predicates (start with today’s: QueueForRegister/Pay no register; Bathroom missing `CanOrderDrink`).

2. **Add staged transitions**
   - Add `next_state` storage (either on `IsAIControlled` or a companion component).
   - Add helpers: `set_next_state(...)`, `has_next_state()`, `clear_next_state()`.
   - Add `afterhours::tags::AITransitionPending`.
   - Update AI call sites (temporary): replace direct `set_state(...)` with `set_next_state(...)` + set `AITransitionPending`.

3. **Add commit + reset**
   - Implement `AICommitNextStateSystem` (clears `AITransitionPending`, sets `AINeedsResetting`).
   - Add force-leave override inside the commit step.
   - Commit behavior:
     - If force-leave is active (Option A: query Sophie + `HasDayNightTimer`), override to `State::Leave`.
     - If `next_state` exists, set `state = next_state`.
     - Clear `next_state` and clear `AITransitionPending`.
     - Set `afterhours::tags::AINeedsResetting` (always).

4. **Add reset-on-enter**
   - Implement `AIOnEnterResetSystem` (clears `AINeedsResetting`).
   - Apply the minimal target/scratch resets needed for the new `state`.

5. **Add fallback**
   - Implement `AIFallbackToWanderSystem` to request Wander when progress is blocked (and no pending transition exists).
   - Register fallback early so it can set `AITransitionPending` and cause other systems to skip.

6. **Port behaviors**
   - Convert `process_state_*` blocks into behavior systems that call `set_next_state(...)` and set `AITransitionPending`.
   - Add `whereNotTag(AITransitionPending)`-style skipping to non-override systems.
   - Implement `NeedsBathroomNowSystem` as an override that can set `next_state` even if `AITransitionPending` is already set.
   - (Current clean-vomit behavior) ensure the entity alternates: CleanVomit → (fallback) Wander → resume_state (CleanVomit).

7. **Register systems**
   - Keep registration in one place (e.g. `system_manager::ai::register_ai_systems`).
   - Register in deterministic order (fallback early, commit last).
   - Ensure force-leave override is enforced at commit time (so registration order doesn’t matter for it).

## Success criteria (definition of done)

- `ai_system.cpp` no longer contains monolithic “process AI for entity” dispatch logic; it becomes a set of small systems + helpers.
- Adding a new AI state is “add a component + add a small system class + register it”.
- Cooldown behavior remains unchanged in Phase 1.
- System ordering is explicit and stable; multi-processing is prevented once we scale.

