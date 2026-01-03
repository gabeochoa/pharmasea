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
- Global state like `g_trigger_fired_while_occupied` to gate fire-once behavior
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
6. **AI Controller**: one component representing AI state + short-term memory, with systems that implement behaviors

---

## Component consolidation plan

### A) Holding / attachments consolidation

#### Update (design preference): keep explicit “CanHoldX” components

The generic `Slot`/`AttachmentSockets` model is optional. If the goal is **simplicity and readability in gameplay code**, keep the current components because they communicate intent extremely well:

- **`CanHoldItem`**: “this entity can hold an item”
- **`CanHoldFurniture`**: “this entity can hold furniture”
- **`CanHoldHandTruck`**: “this entity can hold a handtruck”
- **`CanBeHeld`** (and optionally `CanBeHeld_HT`): “this entity can be carried / is currently carried”

Also: some entities need *multiple* of these simultaneously (e.g. player can hold an item + hold/drag a handtruck; a handtruck can hold furniture; furniture can hold an item), and keeping separate components makes that obvious and easy to reason about.

#### What we actually consolidate: the duplicated systems + special-cases

Instead of replacing the components, consolidate the **implementation**:

- **Unify held-position updates into one system**
  - Replace `UpdateHeldItemPositionSystem`, `UpdateHeldFurniturePositionSystem`, and `UpdateHeldHandTruckPositionSystem` with a single **`HeldAttachmentUpdateSystem`** that:
    - updates any held child transform (item/furniture/handtruck) using shared helpers
    - uses consistent “socket” rules for placement (front/hand/top/behind), but those rules can remain *internal* (no new public component vocabulary required)
    - continues to support `CustomHeldItemPosition` where you want explicit tuning (conveyor vs blender vs table), without forcing everything into a generic slot model

- **Simplify collision behavior with one rule**
  - Keep `CanBeHeld` as the mechanism:
    - if `CanBeHeld::is_held()` → **non-collidable**
  - Move the remaining exceptions out of hardcoded entity-type branches (e.g. rope/mopbuddy-holder edge cases) into either:
    - a small `CollisionOverrides`/`CollisionCategory` component, or
    - a small table keyed by `EntityType` (data-driven), so the collision function stays simple.

- **Rope: keep as a domain feature**
  - `HasRopeToItem` can stay if it remains a special-case mechanic (not “just another hold”).
  - The win is to ensure rope positioning also uses the same shared “follow/offset” helpers as held items, so it stops being its own bespoke transform math path.

#### Optional later step (only if needed)

If/when the “CanHoldX” approach starts to fragment again (new carryables, new attachment-like mechanics), you can still introduce a generic attachment core *underneath* while keeping the `CanHoldItem`-style API surface. But it’s not required to get the simplicity wins right now.

---

### B) AI consolidation

#### Replace these components

- `CanPerformJob` (job enum as the “real AI state”)
- `AIWandering`, `AIWaitInQueue`, `AIDrinking`, `AIUseBathroom`, `AIPlayJukebox`, `AICloseTab`, `AICleanVomit`
- (optionally) `AIComponent` if it becomes redundant

#### With these components

- **`AIController`**
  - **Fields**:
    - `state` (enum: Wander, Queue, Order, Drink, Bathroom, Pay, Leave, …)
    - `cooldown_remaining`
    - `target_entity_id` (optional)
    - `target_pos` (optional)
    - `timer_remaining` (optional)
    - `memory` struct (small: last_register_id, drinks_in_bladder, etc.)
- **`Customer`**
  - Customer-specific data: patience, order, traits (loud/rude/speedwalker), bladder progress.
  - The goal is to move “customer-ness” out of scattered components and into one place.
- **`AgentTraits`** (optional)
  - Generic attribute flags/modifiers (speed multiplier, patience decay multiplier, thief flag, VIP flag).

#### Payoff for upcoming features

- **Thief AI** becomes a new `AIController.state` + a new behavior handler, not “add another AI component + touch many places”.
- **VIP area** becomes an Area policy + a trait + routing rule.
- **Karaoke / order mimicry** becomes a modifier at “choose next order” time, not special cases across systems.

---

### C) “Work” / interaction consolidation

#### Replace

- `HasWork` as “arbitrary lambda attached to entities”

#### With

- **`Interactable`**
  - **Fields**: `interaction_type` (enum), `progress` (0..1), `rate`, `requirements` (held tool? time of day?), `ui_policy`.
- **`Cooldown`** (generic)
  - Used by interactables, trigger areas, fishing game, etc.
- **`Progress`** (generic)
  - One consistent model for progress bars across the world.

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

- **`Area`**
  - **Fields**: `shape` (AABB/circle), `policy` (enum), `progress`, `cooldown`, `required_entrants`, `entrants_scope` (All/One/AllInBuilding), `building` optional.
- **`AreaAction`**
  - **Fields**: `action_type` (enum), `action_payload` (small POD: ints/enums)
  - Example actions: `ChangeGameState`, `LoadSaveSlot`, `StoreReroll`, `PickProgressionOption`, `StartMinigame`, `TeleportPlayers`.
- **`AreaRuntime`**
  - Tracks occupant count, “fired_while_occupied” flag, last_fire_time, etc.

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

- Resolve slot-held entities to `AttachmentParent` links
- Apply socket transforms to children every frame
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

- One authoritative state machine (in `AIController`)
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
- declares `Interactable` configs
- declares `Area` configs (for triggers/markers)

Then keep *only* special construction code when a prefab truly needs runtime binding (rare).

This directly addresses the “growing factory complexity” problem.

---

## How to phase this refactor (recommended order)

### Phase 1: Attachments + Slots (highest leverage)

- Introduce `Slot`, `AttachmentParent`, `AttachmentSockets`
- Implement `AttachmentSystem`
- Migrate:
  - held items
  - held furniture
  - held handtruck
  - (optionally) rope segments
- Delete `CanHoldFurniture` and `CanHoldHandTruck` first (they’re near duplicates).

### Phase 2: Areas/Triggers

- Introduce `Area`, `AreaAction`, `AreaRuntime`
- Implement `AreaSystem` + action dispatcher
- Migrate `IsTriggerArea` and `IsFloorMarker` behaviors into area policies/actions
- Delete trigger globals and the trigger micro-systems

### Phase 3: Interactions / Work

- Introduce `Interactable` + `Progress` + `Cooldown`
- Implement `InteractionSystem` (player-driven)
- Convert the worst offenders first:
  - draft tap / drink filling duplication
  - container cycling/indexing work
  - toilet/vomit cleaning
- Remove `HasWork` lambdas from makers; delete `HasWork` if fully migrated.

### Phase 4: AI consolidation

- Introduce `AIController` + `Customer`
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
  - `Area(policy=MinigameZone)` + `AreaAction(StartMinigame, id=DDRDiscount)`
  - `MinigameSession` component on the player (or global singleton entity) with `Progress/Cooldown`
  - `InteractionSystem`/`AreaSystem` enforces “once per store round” by `Cooldown` or a round-scoped flag

### VIP red carpet area + special ordering rules

- Use:
  - `Area(policy=VIPZone)`
  - `Customer.traits.vip = true` OR assign `AgentTraits{VIP}`
  - AI decision rules read “which areas am I allowed to path to / order from”
  - Pricing system reads “served in VIP zone” multiplier

### Thieves + metal detectors

- Add:
  - `AIController.state = ThiefSteal / ThiefEscape`
  - `Area(policy=DetectorZone)` that fires an action like `FlagCustomerCaught`
  - `Inventory/Slot` primitives already support “pick up bottle” as an attachment/slot transfer

### Bladder / bathroom upgrades (“Break the Seal”, “Luxury Lavatories”)

- `Customer` owns bladder counters.
- Bathroom usage becomes:
  - `AIController.state = Bathroom`
  - `Interactable(type=UseBathroom)` for toilet entity (or “BathroomZone” area policy)
- Upgrade modifiers become data applied to `Customer` needs, not ad-hoc checks in AI components.

### Dynamic prices / queue-length pricing

- With a unified `Customer` and unified queue representation, pricing system can be:
  - `PriceSystem`: reads `QueueState` length at register and applies config multipliers.

### Multi-floor / capacity / “fire code”

- `Area(policy=CapacityZone)` can count occupants and enforce “wait outside” by AI state transitions.
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
  - AI state should live in `AIController`, not split between `CanPerformJob` + many AI components.
  - Trigger gating should live in `AreaRuntime`, not file-static globals.

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

