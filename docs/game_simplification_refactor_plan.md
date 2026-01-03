# Game Simplification Plan (Blue-sky Target; Breaking Changes OK)

This is the **single canonical plan**: it combines the “what’s wrong today” review with a **blue-sky ideal end state**, assuming we are free to make **breaking changes** (component removals/renames, system rewrites, factory redesign).

## Goals

- Reduce the number of gameplay concepts to a small set of reusable primitives.
- Make behavior live in **systems** (not in `entity_makers.cpp` lambdas).
- Make interaction/holding/work **one consistent pipeline**.
- Make new content creation **data-driven** (archetypes), not “edit 6 switches”.

---

## Current state (grounded in `src/entity_makers.cpp` + `src/system/*`)

### What entities are made of today

From `src/entity_makers.cpp`:

- **Baseline**: nearly everything has `Transform`.
- **Players**: input (`CollectsUserInput`, `RespondsToUserInput`), interaction (`CanHighlightOthers`), carrying (`CanHoldItem` + `CanHoldFurniture` + `CanHoldHandTruck`), identity (`HasName`, `HasClientID`), plus movement/render.
- **Customers**: movement + job (`CanPerformJob`, `CanPathfind`) plus many per-behavior AI components.
- **Furniture**: “base furniture” plus optional pieces (`CanHoldItem`, `HasWork`, `IsRotatable`, `CanBeHeld`, `CanBeHighlighted`, etc.).
- **Machines**: implemented mostly as `HasWork` lambdas inside factories, with some machine logic duplicated elsewhere.

### Biggest complexity hotspots

- **Interaction/holding is fragmented**:
  - 3 different “carry slots” (`CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck`)
  - 2 “held flags” (`CanBeHeld`, `CanBeHeld_HT`)
  - held-by semantics also exist in `IsItem`
  - multiple systems update held transforms, and input contains rules about what collides when held
- **Machine logic is duplicated**:
  - draft tap “beer add” exists both in draft tap `HasWork` and drink `HasWork` (two sources of truth)
  - “auto add ingredient” exists as systems (soda fountain) and as work lambdas elsewhere
- **Trigger areas and containers are split into many micro-systems** with similar gating and repeated queries.
- **Factories are doing too much**:
  - they pick renderers, wire behavior via lambdas, and own a huge `convert_to_type()` switch.

---

## The blue-sky perfect end state (minimal concepts)

### 1) Minimal component model

#### Core
- `Transform`
- `Tags` (category flags)

#### Physics
- `Collider` (shape + layer/mask)
- `KinematicBody` (velocity / desired motion)

#### Interaction + carrying (the big simplifier)
- `Interactor` (reach + current focus + intent)
- `Interactable` (what interactions are allowed + requirements)
- `Inventory` (slots, e.g. hand/tool/carry)
- `Attachment` (child → parent + socket/offset rule)

This replaces the entire “holding ecosystem”:
- `CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck`
- `CanBeHeld`, `CanBeHeld_HT`
- “held-by” state in `IsItem`
- multiple held-position systems

#### Work
- `Workable` (progress + action id + UI flags)

No callbacks stored in components.

#### Machines / crafting
- `Machine` (type id + input/output rules + timing)
- `DrinkState` (ingredients + metadata)
- `IngredientSource` (for bottles/tools that add ingredients; uses remaining; validation id)

#### AI
- `Agent` (single customer AI state machine data: state, targets, timers, memory)

#### Economy / store / progression
- `Wallet`
- `Priced`
- `StoreItem`
- `WorldTime` (singleton-like)
- `RoundRules` (singleton-like)

---

### 2) System architecture (one pipeline per domain)

#### World / state
- `WorldTimeSystem` (advance time, emit transition events)
- `RoundSystem` (upgrades/unlocks/spawning rules)

#### Input + interaction (one consistent pipeline)
- `InputSystem` → writes intent into `Interactor`
- `FocusSystem` → picks best `Interactable` per interactor
- `InteractSystem` → applies grab/place/use/work with consistent validation
- `AttachmentSystem` → updates transforms for attachments

#### Movement / physics
- `MovementSystem` (intent → velocity)
- `CollisionSystem` (resolve using collider masks)
- `NavSystem` (pathfinding requests + follow-path for AI)

#### Machines / crafting
- `MachineSystem` (machine execution, insertion/extraction rules)
- `CraftingSystem` (apply ingredient additions, consume uses, update `DrinkState`, emit sound/FX events)

#### AI
- `CustomerAISystem` (single state machine: queue → drink → pay → leave + side-quests)
- `QueueSystem` (shared line mechanics)

#### UI
- `UIHintSystem` (derives progress bars/speech bubbles/icons from state)
- `RenderPrepSystem` (resolve model variants)
- `RenderSystem` / `UISystem`

---

### 3) Entity creation: archetypes, not switches

Replace `convert_to_type()` and most of `entity_makers.cpp` with **archetype definitions**:

- `Archetype { components + defaults + tags }`
- Optional “constructor hook id” for the rare cases that truly require code.

This makes adding content look like:

- Add archetype data
- Add or reuse a machine/work/interaction rule id
- Done

---

## Concrete breaking changes (mapping old → new)

### Holding / interaction

- **Delete**: `CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck`
- **Delete**: `CanBeHeld`, `CanBeHeld_HT`
- **Delete or repurpose**: `IsItem` “held by” semantics (replace with `Attachment`)
- **Replace** with: `Inventory` + `Attachment` + unified `InteractSystem`

### Work / machines

- **Delete**: `HasWork` callbacks/lambdas as the primary behavior mechanism
- **Replace** with: `Workable(action_id)` and a small action table in systems
- **Unify**: soda fountain, draft tap, ice machine, squirter, blender as `Machine(type_id)`

### AI

- **Delete**: per-behavior AI components (`AIWaitInQueue`, `AIDrinking`, etc.)
- **Replace** with: `Agent` (single component) + `CustomerAISystem`

### State

- **Replace** the “Sophie mega-entity owns everything” pattern with:
  - `WorldTime` + `RoundRules` on a dedicated world entity (or globals managed by systems)

### Systems cleanup

- **Merge** trigger area micro-systems into one `TriggerAreaSystem`.
- **Merge** container micro-systems into one `ContainerMaintenanceSystem` (or subsystems inside one file).
- **Centralize** gating (day/night/transition) into one shared run-context.

---

## Recommended “perfect” build order (still phased, but breaking is fine)

This is the fastest route to “everything feels simpler” (not the safest, but the cleanest).

### Step 1 — Unify interaction + carrying

Implement:
- `Interactor`, `Interactable`, `Inventory`, `Attachment`
- `FocusSystem`, `InteractSystem`, `AttachmentSystem`

Then delete the legacy holding ecosystem and remove special casing from input/collision.

### Step 2 — Move work out of factories

Implement:
- `Workable(action_id)` + `WorkSystem`

Convert:
- vomit cleaning
- toilet cleaning
- interactive settings changer
- draft tap fill

### Step 3 — Unify machines into one crafting/machine model

Implement:
- `Machine(type_id)` + `MachineSystem` + `CraftingSystem`

Convert:
- soda fountain
- ice machine
- draft tap
- blender + fruit → juice

This fixes duplicated logic (like beer tap) by construction.

### Step 4 — AI consolidation

Implement:
- `Agent` + `CustomerAISystem`

Convert customer behavior to a single state machine.

### Step 5 — Archetypes

Replace `convert_to_type()` with a registry/archetype table.

---

## “If we only do one thing” recommendation

Build the **unified Interaction + Inventory + Attachment pipeline** first.

It collapses the biggest current complexity cluster (holding/highlighting/pickup/drop/handtruck special cases) and makes machines + AI refactors dramatically simpler.

