## AI Refactor Plan (Split-out)

This document is a deeper dive on the AI portion of `ecs_architecture_simplification_refactor_plan.md`.

### Goals

- **Make AI state easy to reason about**: avoid “god components” and avoid “hidden state” spread across unrelated places.
- **Preserve your current ergonomics**: it should remain easy to “reset just Queue” or “reset just Drinking”.
- **Remove duplication**: one clear state machine + a small number of explicit per-state scratch components.
- **Compose with the rest of the ECS**: AI should use the same holding/interactions/areas primitives as players.

### Non-goals

- Snapshot compatibility.
- Perfectly minimal changes.

---

## Current state (what we’re simplifying)

Today the customer AI is effectively:

- High-level state: `CanPerformJob::current` (JobType enum)
- Per-job components: `AIWaitInQueue`, `AIWandering`, `AIDrinking`, `AIUseBathroom`, etc.
- Per-job helper structs (e.g. `AITarget`, `AILineWait`, `AITakesTime`) embedded inside those components
- “Cooldown” semantics implemented by `AIComponent`

This is already modular, but it mixes:

- “What am I doing?” (job enum)
- “How do I do it?” (components)
- “Who am I?” (patience/order/bladder etc split across multiple components)

---

## Proposed component model (explicit + reset-friendly)

### 1) Identity + durable data

- **`IsCustomer : BaseComponent`**
  - Durable “customer-ness” that must survive task resets.
  - Examples:
    - current order / order history (if needed)
    - bladder counters / drinks-in-bladder
    - traits/flags (VIP/thief/loud/rude/speedwalker etc) if you don’t want a separate traits component
    - any global multipliers that apply across multiple states

- **`HasAgentTraits : BaseComponent`** (optional)
  - If you want traits reusable for non-customers (robots, special NPCs).

Rule of thumb:
- If resetting a task should *not* wipe it, it belongs in `IsCustomer` (or a durable “HasX”).
- If resetting a task *should* wipe it, it belongs in a per-state component.

---

### 2) High-level controller state

- **`IsAIControlled : BaseComponent`**
  - Keep this small and obvious.

Suggested fields:
- `State state`
- (optional) `State resume_state` if you like the “wander as a pause then resume” behavior

Notes:
- This replaces the “job enum” as the single authoritative driver.
- It should not hold timers, targets, or per-state scratch.

---

### 3) Targeting primitives (split, explicit)

- **`HasAITargetEntity : BaseComponent`**
  - `EntityRef entity{};`
  - Meaning: “my current target entity” (register/toilet/jukebox/mess/etc).

- **`HasAITargetLocation : BaseComponent`**
  - `std::optional<vec2> pos;`
  - TODO in code: replace `optional<vec2>` with a `Location` typedef.
  - Meaning: “my current target point in world space”.

Why split entity vs location:
- No sentinel values / no “has_target_pos” booleans.
- Systems can require exactly what they need.
- Reset is trivial: clear/remove the one you want.

---

### 4) Time primitives (shared semantics without inheritance)

Use a shared non-component struct:

- **`CooldownInfo`** (not a component)
  - `remaining`, `reset_to`, `enabled`, `tick()`, `ready()`, `reset()`, `clear()`

Then embed it into components:

- **`HasCooldown : BaseComponent`**
  - generic gating timer for “use/trigger allowed?” (often lives on interactables/areas)

- **`HasAICooldown : BaseComponent`**
  - decision pacing timer for “think/retarget allowed?”

- **`HasActionTimer : BaseComponent`** (optional)
  - explicit “this state takes time” timer (drinking duration, bathroom duration, paying duration)

Guidance:
- Don’t try to unify AI pacing and “takes time” into one component unless you’re sure you never need both simultaneously.
- Using multiple components that embed the same `CooldownInfo` keeps the semantics shared but the storage independent.

---

### 5) Per-state scratch (the reset lever)

Only add these when they genuinely save complexity.

- **`HasAIQueueState : BaseComponent`**
  - `EntityRef last_register{};` (anti-thrash / stability)
  - `int queue_index = -1;` (if you cache it)
  - (standing location should come from `HasAITargetLocation::pos`)

- **`HasAIJukeboxState : BaseComponent`**
  - `EntityRef last_jukebox{};` (optional stability)

What we intentionally removed:
- Bathroom “last used toilet” state: pick closest available, store active target in `HasAITargetEntity`.
- Wander “goal”: use `HasAITargetLocation` directly.

Reset semantics:
- “Reset queue behavior” = remove `HasAIQueueState` + clear `HasAITargetEntity`/`HasAITargetLocation`.
- “Reset jukebox behavior” = remove `HasAIJukeboxState` + clear `HasAITargetEntity`.

---

## System design (how the AI runs)

### AISystem (single dispatcher)

One system should drive AI each frame (or at a fixed tick) in a predictable way:

- Early-out if entity is not `IsAIControlled`
- If entity has `HasAICooldown` and not ready → tick cooldown and return
- Switch on `IsAIControlled::state`:
  - call behavior handler (QueueBehavior, DrinkBehavior, BathroomBehavior, etc.)

Each behavior handler is responsible for:
- setting/validating `HasAITargetEntity` or `HasAITargetLocation`
- interacting with world systems by *writing components* (e.g. request interaction, or directly call an interaction helper)
- updating `IsAIControlled::state` transitions

### “Keep target until invalid” rule (stability)

To avoid thrash without storing “last used”:

- If `HasAITargetEntity` exists and the entity is still valid/usable, keep it.
- Only reselect when:
  - target entity destroyed/unavailable
  - path no longer valid
  - state transition requires a new target

This is the main stability mechanism.

---

## How AI composes with the rest of the ECS

### Holding (A)
AI should manipulate the same holding components the player uses:

- reads `CanHoldItem` to see what it’s carrying (if AI can carry)
- writes `CanHoldItem` (or uses the same pickup/drop helpers) to pick up drinks, etc.

### Interactions (C)
Prefer AI to use the same interaction system by:

- selecting a target `IsInteractable`
- moving into range (via pathfinding)
- triggering an interaction request (either by writing an “AI wants to interact” component/event, or by calling a shared helper)

This avoids duplicating “how to use toilet/jukebox/register”.

### Areas (D)
AI can react to areas by:

- state transition rules (“if in VIP zone, choose VIP register”)
- area constraints (“capacity zone full → wait outside”)

---

## Migration plan (from current code)

1. **Introduce `IsAIControlled`** and map it 1:1 to existing `CanPerformJob::current` initially.
2. **Keep existing AI behavior code**, but route entry through one dispatcher that switches on `IsAIControlled::state`.
3. **Introduce `IsCustomer`** and move durable data into it incrementally:
   - bladder counters first
   - order metadata
   - traits
4. **Replace per-behavior embedded target structs** with `HasAITargetEntity` / `HasAITargetLocation` where it simplifies call sites.
5. **Replace `AIComponent` cooldown with `HasAICooldown{CooldownInfo}`** (or keep `AIComponent` but implement it via `CooldownInfo`).
6. When stable, delete `CanPerformJob` and the old `AI*` components that are no longer needed.

---

## Open questions (intentional)

- Do we want AI to always use the interaction system, or do we allow some behaviors to “directly mutate” state for simplicity?
- How much of queue state should be stored vs recomputed each tick?
- Do we need separate AI tick rate (e.g. fixed 10Hz) for determinism/perf?

