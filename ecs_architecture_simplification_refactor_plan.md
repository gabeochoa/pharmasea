## ECS Architecture Simplification Refactor Plan

### Why this plan exists
The current ECS shape works, but it’s trending toward “death by a thousand tiny components/systems”:

- **Duplicated work logic**: interaction code lives inside `entity_makers.cpp` via `HasWork` lambdas (e.g., drink-making + draft tap logic duplicated, container cycling duplicated, etc.), making gameplay rules hard to find and easy to fork accidentally.
- **Growing factory complexity**: `src/entity_makers.cpp` is both a prefab library *and* a gameplay rules engine (validation, progress rules, sound effects, state gating).
- **Scattered system logic**: key domains (trigger areas, containers, attachments/held positioning) are split across many micro-systems and helper functions across multiple files.
- **Too many “tag” components**: many `IsX` components encode “what this entity is” even though `EntityType` already exists, which increases conceptual overhead.

This plan proposes **breaking changes** (per your note) to get the codebase to a simpler mental model.

---

## Goals

- **Fewer concepts**: reduce the number of component “kinds” the game uses to express the same idea (holding/attachment, AI job/state, interactables).
- **Make rules discoverable**: gameplay logic should live in systems/modules, not hidden inside factories.
- **Reduce duplicated gates**: eliminate repeated `should_run()` patterns that all check the same game/phase/timer flags.
- **Make future features cheap**: upcoming mechanics (VIP zone, thief AI, bladder, karaoke/rhythm area, dynamic pricing) should drop into existing primitives (areas, interactions, AI states, modifiers).

## Non-goals (for this refactor)

- **Snapshot/schema compatibility**: explicitly not preserved here.
- **Perfectly minimal diff**: correctness + clarity over small patches.
- **Performance micro-optimizations**: we’ll keep reasonable query hygiene, but simplicity wins.

---

## Current pain points (concrete examples from the code)

### 1) “Work” / interactions are defined in factories
`HasWork` stores a function pointer (`WorkFn`) which cannot be serialized and is wired up in makers. This causes:

- The “real rule” to be split across *factory code* + *systems that invoke work*.
- Duplicated work logic:
  - Draft tap behavior appears in both `furniture::make_draft()` and `items::process_drink_working()` in `src/entity_makers.cpp`.
  - Container “cycle index / respawn held item” exists in both container `HasWork` and container maintenance systems.

### 2) Holding/attachment is modeled three different ways
Examples:

- **Held item**: `CanHoldItem` + `IsItem` + `CustomHeldItemPosition` + `UpdateHeldItemPositionSystem`.
- **Held furniture**: `CanHoldFurniture` + `UpdateHeldFurniturePositionSystem`.
- **Held handtruck**: `CanHoldHandTruck` + `UpdateHeldHandTruckPositionSystem`.
- **Rope chain**: `HasRopeToItem` + `ProcessHasRopeSystem`.
- Collisions special-case held objects in `input_process_manager::is_collidable()`.

Net effect: “what is attached to what?” is spread across components, special-cases, and multiple systems.

### 3) Trigger areas are split across many systems + globals
Trigger logic currently spans:

- `IsTriggerArea` data (progress/cooldown/entrants rules)
- Multiple per-frame systems in `afterhours_sixtyfps_systems.cpp` (counting entrants, progress, callbacks)
- Large callback switch logic that lives far from the trigger component/system (and mixes store/progression/state transitions)

### 4) AI is both “state in components” and “state in a job enum”
Right now:

- AI behavior is fragmented across many components (`AIWandering`, `AIWaitInQueue`, `AIDrinking`, etc.)
- But the actual state machine is driven by `CanPerformJob::current` (job enum), and AI systems branch on that.

This duplicates state, complicates debugging, and makes adding new behaviors (thief, VIP, karaoke mimicry) more expensive than it needs to be.

---

## Target ECS shape (proposed primitives)

This refactor centers the game around a few reusable primitives:

1. **Identity/Tags**: “What is this entity?” and “how should it behave in lifecycle/queries?”
2. **Attachment**: one generic way to model parent→child relationships (held items, held furniture, rope segments, etc.)
3. **Slots/Containers**: “this entity can contain/hold another entity” (hand slot, surface slot, machine slot, shelf slot) using one component shape
4. **Interactables**: interaction logic expressed as data + systems, not embedded lambdas
5. **Areas/Zones**: trigger areas, floor markers, VIP zones, etc. represented as area volumes with policies
6. **AI Controller**: one component representing AI state, with systems that implement behaviors

---

## Component consolidation plan

### A) Holding / attachments consolidation

TODO: review `CanHold*` components to see if theres a better way

---

### B) AI consolidation

#### Replace these components

- `CanPerformJob` (job enum as the “real AI state”)
- `AIWandering`, `AIWaitInQueue`, `AIDrinking`, `AIUseBathroom`, `AIPlayJukebox`, `AICloseTab`, `AICleanVomit`
- (optionally) `AIComponent` if it becomes redundant

#### With these components

- **`IsAIControlled`**
  - Keep this as the *authoritative high-level mode* only (small + obvious):

```cpp
struct IsAIControlled : public BaseComponent {
  enum class State {
    Wander,
    QueueForRegister,
    AtRegisterWaitForDrink,
    Drinking,
    Bathroom,
    Pay,
    PlayJukebox,
    CleanVomit,
    Leave,
  } state = State::Wander;

  // Optional: preserve the current “wandering is a pause” behavior.
  State resume_state = State::Wander;
};
```

- **Cooldown/timers (shared semantics, no inheritance)**

```cpp
// Not a component: shared timer semantics used by multiple components.
struct CooldownInfo {
  float remaining = 0.f;   // seconds until ready
  float reset_to = 0.f;    // seconds to reset to when triggered
  bool enabled = true;

  void tick(float dt) {
    if (!enabled) return;
    if (remaining > 0.f) remaining -= dt;
  }
  [[nodiscard]] bool ready() const { return !enabled || remaining <= 0.f; }
  void reset() { remaining = reset_to; }
  void clear() { remaining = 0.f; }
};

struct HasAICooldown : public BaseComponent {
  CooldownInfo cooldown{};
};
```

- **`HasAITargetEntity`** + **`HasAITargetLocation`**
  - Split targeting into two clear primitives so systems depend only on what they need:
    - target-by-entity (register/toilet/jukebox/etc)
    - target-by-world-location (a point to path toward / stand on)

```cpp
struct HasAITargetEntity : public BaseComponent {
  EntityRef entity{};
};

struct HasAITargetLocation : public BaseComponent {
  // TODO replace optional<vec2> with Location typdef
  std::optional<vec2> pos;
};
```

- **Per-state scratch components (recommended)**
  - This preserves what you like today: “reset just the Drinking task” without affecting wandering/queue/etc.
  - Add only the ones you actually need:

```cpp
struct HasAIQueueState : public BaseComponent {
  EntityRef last_register{};   // helps avoid thrash
  int queue_index = -1;        // last known position in line (optional)
};

struct HasAIJukeboxState : public BaseComponent {
  EntityRef last_jukebox{};
};
```

Notes:
TODO: finalize which AI data belongs in `IsCustomer` vs which should live on per-state components, and define a consistent reset pattern (e.g., clearing `HasAITargetEntity`/`HasAITargetLocation` vs removing state components).

#### Payoff for upcoming features

TODO: validate this AI split against planned features (thief/VIP/karaoke) and ensure each feature only needs:
- adding a new state + behavior handler, and/or
- adding a small `HasX` config component,
without expanding shared “god components”.

---

### C) “Work” / interaction consolidation

#### Replace

- `HasWork` as “arbitrary lambda attached to entities”

#### With

- **`IsInteractable`**
  - Proposed explicit shape (illustrative; the point is to be *data-first* and remove lambdas from `HasWork`):

```cpp
// Minimal reusable progress bar backing for anything interactive.
struct HasProgress : public BaseComponent {
  float pct = 0.f;                 // 0..1
  bool hide_when_full = false;
  bool hide_always = false;
};

// Generic cooldown primitive (also used by triggers/minigames/etc).
struct HasCooldown : public BaseComponent {
  CooldownInfo cooldown{};
};

// Optional: if you want a distinct “this state/action lasts N seconds” timer
// without conflating it with “cooldown gating”, keep it as its own component.
struct HasActionTimer : public BaseComponent {
  CooldownInfo timer{};
};

// A data-driven “do work” target. Systems implement the enum behaviors.
struct IsInteractable : public BaseComponent {
  enum class Type {
    None,

    // Replaces container “cycle indexer” work
    CycleVariant,

    // Station-style interactions (draft tap / soda fountain / squirter)
    DispenseIngredient,

    // Applies AddsIngredient from the item you’re holding
    AddIngredientFromHeldTool,

    // Cleanup interactions
    CleanMess,

    // Minigames / one-shot interactions
    PlayMinigame,

    // Toggling/setting changes
    ToggleSetting,
  } type = Type::None;

  // How fast the interaction progresses when held (pct/sec).
  float rate = 1.f;

  // Requirements are intentionally explicit + boring.
  enum RequirementFlags : uint32_t {
    ReqNone            = 0,
    ReqServerOnly      = 1 << 0,  // authority check
    ReqBarClosedOnly   = 1 << 1,  // planning/store only
    ReqBarOpenOnly     = 1 << 2,  // in-round only
    ReqHoldingItem     = 1 << 3,  // player must have CanHoldItem non-empty
    ReqHoldingMop      = 1 << 4,  // example: cleanup faster/allowed
  };
  uint32_t requirements = ReqNone;

  // Optional: what input drives it (defaults to “do work”).
  enum class InputGate { DoWorkHeld, DoWorkPressed } input_gate =
      InputGate::DoWorkHeld;

  // UI policy: keeps this component self-contained.
  enum class UiPolicy { DefaultProgressBar, Hidden, Custom } ui_policy =
      UiPolicy::DefaultProgressBar;

  // Tiny payload for parameterizing the behavior without lambdas.
  // (Keep it small; if it grows, split into per-type HasX components.)
  int param_i32 = 0;
};
```

Notes:
- `HasProgress`/`HasCooldown` are separate so they can be shared by triggers, fishing, etc.
- If some interactions need richer config, prefer adding a small `HasX` component (e.g. `HasDispenseIngredientConfig`) rather than inflating `IsInteractable`.
  - Uses `HasProgress` and `HasCooldown` (both reusable by triggers/minigames/etc).

#### Example interaction types (data-driven)

- `CycleVariant` (replaces container “cycle indexer” work)
- `DispenseIngredient` (draft tap / soda fountain / squirter)
- `AddIngredientFromHeldTool` (adds ingredient from held item like fruit/alcohol/simple syrup)
- `CleanMess` (vomit/toilet cleaning)
- `PlayMinigame` (fishing/champagne open / DDR discount)
- `ToggleSetting` (interactive setting changer)

The key is: **systems implement interaction types**; entity makers only configure which type applies.

---

### D) Areas / triggers consolidation

#### Replace

- `IsTriggerArea`
- `IsFloorMarker`
- global trigger gating state (e.g. “fired while occupied” sets)

#### With

- **`IsArea`**
  - **Fields**: `shape` (AABB/circle), `policy` (enum), `progress`, `cooldown`, `required_entrants`, `entrants_scope` (All/One/AllInBuilding), `building` optional.
- **`HasAreaAction`**
  - **Fields**: `action_type` (enum), `action_payload` (small POD: ints/enums)
  - Example actions: `ChangeGameState`, `LoadSaveSlot`, `StoreReroll`, `PickProgressionOption`, `StartMinigame`, `TeleportPlayers`.
- **`HasAreaRuntime`**

This converts triggers from “distributed logic + globals + large switch statements” into a single **AreaSystem** that:

- Computes occupants
- Advances progress/cooldown
- Fires actions via a small action dispatcher

---

### E) Rendering/UI component cleanup (optional but synergistic)

Right now visuals are split across `ModelRenderer`, `SimpleColoredBoxRenderer`, `HasDynamicModelName`, `UsesCharacterModel`, plus special-case systems.

Proposed:

- **`Renderable`**: one component that can represent “model by name” or “primitive box”.
- **`VisualVariant`**: one component that encodes “how to pick a model variant” (by ingredients, by open/closed, by subtype).
- **`WorldLabel`**: unify name/speech bubble/progress text into one component + one render system.

This isn’t required for gameplay simplification, but it reduces the “where is this drawn?” hunt.

---

## System consolidation plan

### 1) AttachmentSystem (new, central)

**Replaces**:

- `UpdateHeldItemPositionSystem`
- `UpdateHeldFurniturePositionSystem`
- `UpdateHeldHandTruckPositionSystem`
- large parts of collision special-casing for held items
- most “face direction math” scattered across systems

**Responsibilities**:

- Read held relationships from `CanHoldItem` / `CanHoldFurniture` / `CanHoldHandTruck`
- Apply consistent held placement rules to the held entity every frame (front/hand/top/behind + `CustomHeldItemPosition` overrides)
- Apply “non-collidable while attached” via collider flags/categories

---

### 2) ContainerSystem (merge + simplify)

**Replaces / absorbs**:

- `ProcessIsContainerAndShouldBackfillItemSystem`
- `ProcessIsContainerAndShouldUpdateItemSystem`
- `ProcessIsIndexedContainerHoldingIncorrectItemSystem`
- `system_manager::fix_container_item_type()`-style fixups

**Responsibilities**:

- Ensure container config is correct (by archetype/prefab)
- Handle refill/backfill policies
- Handle cycling variant/index changes
- Validate held item matches container policy

Net effect: container behavior is in one place regardless of game phase (planning/inround/modeltest).

---

### 3) AreaSystem (merge all trigger/marker logic)

**Replaces**:

- `UpdateDynamicTriggerAreaSettingsSystem`
- `Count*TriggerAreaEntrantsSystem`
- `UpdateTriggerAreaPercentSystem`
- `TriggerCbOnFullProgressSystem`
- `ClearAllFloorMarkersSystem` / `MarkItemInFloorAreaSystem` (if migrated to `Area` policies)

**Responsibilities**:

- Occupant counting (with policy: All, One, AllInBuilding)
- Progress/cooldown integration
- Fire gating (once-per-occupancy)
- Action dispatch

---

### 4) InteractionSystem (work/do-work consolidation)

**Replaces**:

- The “HasWork::call() is invoked from X” pattern
- Many bespoke “do work” branches in input code and entity makers

**Responsibilities**:

- Determine current interact target (highlighted/nearest/in-front)
- Validate requirements (held tool, day/night, etc.)
- Advance progress
- Fire the interaction effect

This makes “how do I interact with machines?” centralized and easy to change.

---

### 5) TransportSystem (optional consolidation)

You can keep multiple transport subsystems, but they should share:

- common gating (phase)
- common slot/attachment primitives
- shared filtering helpers

Candidates to consolidate:

- conveyor + grabber + filtered grabber
- pneumatic pipe pairing + movement
- rope behavior (if rope stays as a “transport” concept)

If you want the simplest conceptual model, merge them into a single “ItemTransportSystem” with internal phases.

---

### 6) AISystem (single controller state machine)

**Replaces**:

- `ProcessAiSystem` branching on `CanPerformJob`
- behavior components per-job

**Responsibilities**:

- One authoritative state machine (in `IsAIControlled`)
- Behavior handlers as functions/modules (QueueBehavior, DrinkBehavior, BathroomBehavior, ThiefBehavior, etc.)
- Separation between “decision” (choose next) and “actuation” (move, interact, pay)

---

## Refactor `entity_makers.cpp` (factory simplification)

### Problem
`entity_makers.cpp` currently does all of:

- prefab assembly
- behavior wiring
- interaction rules via lambdas
- some systems-like logic (e.g., work progress policies)

### Target
Make entity makers mostly “instantiate prefab + configure data”, not “define gameplay logic”.

### Proposal: PrefabLibrary

Create a single table `PrefabLibrary` keyed by `EntityType` (or string prefab ids), where each prefab:

- declares base components
- declares default render/collider config
- declares slots/sockets
- declares `IsInteractable` configs
- declares `IsArea` configs (for triggers/markers)

Then keep *only* special construction code when a prefab truly needs runtime binding (rare).

This directly addresses the “growing factory complexity” problem.

---

## How to phase this refactor (recommended order)

### Phase 1: Attachments + holds (highest leverage)

- Consolidate the “hold” implementation (keep `CanHoldX`, unify the systems)
- Implement `HeldAttachmentUpdateSystem`
- Migrate:
  - held items
  - held furniture
  - held handtruck
  - (optionally) rope segments
- Delete `CanHoldFurniture` and `CanHoldHandTruck` first (they’re near duplicates).

### Phase 2: Areas/Triggers

- Introduce `IsArea`, `HasAreaAction`, `HasAreaRuntime`
- Implement `AreaSystem` + action dispatcher
- Migrate `IsTriggerArea` and `IsFloorMarker` behaviors into area policies/actions
- Delete trigger globals and the trigger micro-systems

### Phase 3: Interactions / Work

- Introduce `IsInteractable` + `HasProgress` + `HasCooldown`
- Implement `InteractionSystem` (player-driven)
- Convert the worst offenders first:
  - draft tap / drink filling duplication
  - container cycling/indexing work
  - toilet/vomit cleaning
- Remove `HasWork` lambdas from makers; delete `HasWork` if fully migrated.

### Phase 4: AI consolidation

- Introduce `IsAIControlled` + `IsCustomer`
- Move logic from `CanPerformJob` branching into controller behaviors
- Remove per-behavior AI components
- Add hooks for future features (thief, VIP, fire code, etc.)

### Phase 5 (optional): Visual/UI cleanup

- Consolidate model/box renderers + dynamic model names under `Renderable` + `VisualVariant`
- Add `WorldLabel` and unify speech bubble/name/progress rendering

---

## What this would look like for near-future features (from `ideas.md`, `todo.md`, `progression_tree.md`, `docs/game_brief.md`)

### Rhythm mixing / DDR discount (store trigger once per round)

- Implement as:
  - `IsArea(policy=MinigameZone)` + `HasAreaAction(StartMinigame, id=DDRDiscount)`
  - `MinigameSession` component on the player (or global singleton entity) with `HasProgress`/`HasCooldown`
  - `InteractionSystem`/`AreaSystem` enforces “once per store round” by `HasCooldown` or a round-scoped flag

### VIP red carpet area + special ordering rules

- Use:
  - `IsArea(policy=VIPZone)`
  - `IsCustomer.traits.vip = true` OR assign `HasAgentTraits{VIP}`
  - AI decision rules read “which areas am I allowed to path to / order from”
  - Pricing system reads “served in VIP zone” multiplier

### Thieves + metal detectors

- Add:
  - `IsAIControlled.state = ThiefSteal / ThiefEscape`
  - `IsArea(policy=DetectorZone)` that fires an action like `FlagCustomerCaught`
  - The existing “hold” primitives already support “pick up bottle” as a transfer

### Bladder / bathroom upgrades (“Break the Seal”, “Luxury Lavatories”)

- `IsCustomer` owns bladder counters.
- Bathroom usage becomes:
  - `IsAIControlled.state = Bathroom`
  - `IsInteractable(type=UseBathroom)` for toilet entity (or “BathroomZone” area policy)
- Upgrade modifiers become data applied to `IsCustomer` needs, not ad-hoc checks in AI components.

### Dynamic prices / queue-length pricing

- With a unified `IsCustomer` and unified queue representation, pricing system can be:
  - `PriceSystem`: reads `QueueState` length at register and applies config multipliers.

### Multi-floor / capacity / “fire code”

- `IsArea(policy=CapacityZone)` can count occupants and enforce “wait outside” by AI state transitions.
- Pathfinding constraints can be expressed as “allowed zones” or “goals” rather than hardcoding building rectangles.

---

## Implementation notes / guardrails

- **Stop encoding “type = behavior” as `IsX` tags** unless the component has *data*.
  - Prefer `EntityType` + prefab config + tag bitsets.
- **Centralize phase gating**
  - Define a single “PhaseContext” each frame (Planning/InRound/Store/Transitioning).
  - Systems read that context instead of duplicating `should_run()` checks on `GameState` + `HasDayNightTimer`.
- **Prefer enums + small payloads over lambdas**
  - Keep the behavior logic in systems; keep entity config as data.
- **Keep one authoritative owner for state**
  - AI state should live in `IsAIControlled`, not split between `CanPerformJob` + many AI components.
  - Trigger gating should live in `HasAreaRuntime`, not file-static globals.

---

## Deliverables checklist (what “done” means)

- **Holding/attachment**
  - One attachment primitive
  - One system that updates all attached children
  - No special-case collision code for held items
- **Triggers/areas**
  - One AreaSystem
  - No global “trigger fired while occupied” state
- **Interactions**
  - No gameplay-rule lambdas in `entity_makers.cpp`
  - Interactions are discoverable in a dedicated system/module
- **AI**
  - One controller component + one state machine
  - Easy to add new behaviors (thief, VIP, karaoke mimicry)
- **Factories**
  - Makers become declarative prefab configuration
  - Large switch-based behavior logic removed from makers

