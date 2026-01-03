# Game Architecture Simplification Plan (Components + Systems)

This document proposes refactors to **simplify the ECS surface area** (fewer concepts, fewer duplicated rules, fewer one-off systems), while staying compatible with the project’s **snapshot/network schema**.

## Goals

- Reduce the number of “concepts” a contributor must understand to add new gameplay.
- Remove duplication across systems (especially state gating and “machine” logic).
- Consolidate components that are essentially the same idea expressed multiple ways.
- Make entity creation (factories) more data-driven and less “giant switch” driven.

## Hard constraints (must respect)

### Snapshot schema is stable and positional

`src/components/all_components.h` defines `snapshot_blob::ComponentTypes` with explicit rules:

- **Never reorder**
- **Only append**
- Deleting/reordering breaks save/replay/network snapshot compatibility

This implies:

- **Renames**: treat as *add new component type + migrate*, don’t “just rename”.
- **Removals**: treat as *stop using + migrate + keep legacy type in schema*.
- **Merges**: create a new unified component, migrate from old at runtime/load, and leave old types present (but unused).

## Current state (as of `src/entity_makers.cpp`)

### Entity composition patterns

`entity_makers.cpp` is effectively the “truth” for what components actually matter:

- **Everyone**: `Transform`
- **People-ish**: `HasBaseSpeed`, renderer (`ModelRenderer` or `SimpleColoredBoxRenderer`), holding/pushing
- **Players**: `CollectsUserInput`, `RespondsToUserInput`, `CanHighlightOthers`, `CanHoldFurniture`, `CanHoldHandTruck`, `HasClientID`, `HasName`
- **AI people**: `CanPerformJob` + `CanPathfind` + a set of per-behavior AI components (e.g. `AIWaitInQueue`, `AIDrinking`, etc.)
- **Furniture base**: `IsSolid`, `CanHoldItem` (optional), `CanBeHeld` / `CanBeHighlighted` (optional), `IsRotatable` (optional), renderer(s)
- **Work machines**: `HasWork` callbacks embedded in `entity_makers.cpp` (ice machine, toilet cleaning, draft tap, etc.)
- **Singleton “game brain” entity (`Sophie`)**: `IsRoundSettingsManager`, `HasDayNightTimer`, `IsProgressionManager`, `IsBank`, `IsNuxManager`, `CollectsCustomerFeedback`

### “Smells” observed directly in `entity_makers.cpp`

- **Work logic duplication**: draft tap logic is duplicated between `make_draft()` and drink `process_drink_working()` (`TODO` notes acknowledge this).
- **Factory growth risk**: `convert_to_type()` is a large switch that will keep growing; it is easy to forget to update.
- **A lot of behavior lives in factories** via lambdas bound into `HasWork`. This makes it hard to “see the systems” as systems.

## Current systems layout (high-level)

From `src/system/system_manager.*` + `src/system/afterhours_*systems.cpp`:

- `register_sixtyfps_systems()` runs a *lot* every frame (trigger areas, highlights, held-item positioning, etc.).
- `register_gamelike_systems()` runs in “game-like states”, with repeated state gating patterns.
- `register_inround_systems()` runs primarily when the bar is open (night/in-round), again with repeated gating patterns.
- Day/night transitions are handled by many small systems gated on `HasDayNightTimer::needs_to_process_change`.
- ModelTest has its own “container maintenance” systems that largely duplicate in-round/planning container behavior.

## Proposed simplifications (highest leverage)

### 1) Consolidate “container maintenance” into one coherent system group

**Problem**

Container logic currently exists in multiple places with slightly different gating:

- `process_is_container_and_should_backfill_item`
- `process_is_container_and_should_update_item`
- `process_is_indexed_container_holding_incorrect_item`
- Plus duplicated versions in `afterhours_modeltest_systems.cpp`

This increases “where should this change go?” confusion and risks subtle state differences.

**Proposal**

Create a single **`ContainerMaintenanceSystem`** (or a small cluster) that owns:

- backfill empty containers
- handle index changes
- resolve “wrong item for current index” cases
- optional: apply the `fix_container_item_type()` mapping once on load / on first tick

And give it a **single source of truth** for when it runs:

- ModelTest: always
- Game-like, bar closed (planning): update index + backfill
- Game-like, bar open (in-round): backfill + enforce invariants
- Transition frames (`needs_to_process_change`): do nothing

**Compatibility note**

This is a pure behavior refactor; no schema changes required.

---

### 2) Collapse trigger-area processing into a single `TriggerAreaSystem`

**Problem**

Trigger areas are implemented as multiple micro-systems (count entrants, update percent, fire callback, update dynamic label/title, etc.). This causes:

- repeated queries over similar data
- ordering coupling spread across files
- harder debugging (which of the 5 systems did the thing?)

**Proposal**

Replace the sequence:

- UpdateDynamicTriggerAreaSettings
- CountAllPossibleTriggerAreaEntrants
- CountInBuildingTriggerAreaEntrants
- CountTriggerAreaEntrants
- UpdateTriggerAreaPercent
- TriggerCbOnFullProgress

with a **single system** that, per `IsTriggerArea` entity, does:

- update settings/title/subtitle
- compute entrants (and “all possible entrants”) once
- update progress/cooldown
- fire callback if complete (with gating)

**Expected benefits**

- fewer update systems to reason about
- fewer entity queries / less duplicated work
- clearer invariants (“trigger areas update happens here”)

---

### 3) Unify “held state” and “holdable” concepts (reduce component count + special casing)

**Problem**

There are multiple overlapping representations:

- `IsItem` tracks held-by (holder type + id), but does not serialize held-by fields.
- `CanBeHeld` is used to mark “furniture being held”.
- `CanBeHeld_HT` exists solely for hand-truck-style holding (and is checked separately in collision code).
- Three separate “carry slots” exist (`CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck`), each with its own transform-follow system.

This spreads “what’s held / what collides / what follows me” across:

- input code (`input_process_manager.cpp`)
- multiple update systems (held item, held furniture, held handtruck)
- collisions (“disable collision when held”) checks

**Proposal**

Introduce a unified model:

- **`Holdable`** (new) for anything that can be carried/attached; contains:
  - `bool held`
  - optional `EntityRef holder`
  - optional `HoldSlot slot_kind` (Item/Furniture/HandTruck/etc.)
- **`AttachmentSlots`** (new) on holders; contains N slots (`EntityRef child`, placement rule).
- One **`UpdateAttachmentsSystem`** that updates child transforms for all slots.

**Migration approach (schema-safe)**

- Append `Holdable` + `AttachmentSlots` to `snapshot_blob::ComponentTypes`.
- Add a migration system:
  - if entity has `CanBeHeld_HT` but not `Holdable`, create `Holdable` and copy `held`
  - similarly for `CanBeHeld`
  - for holders, translate from `CanHoldItem`/`CanHoldFurniture`/`CanHoldHandTruck` into `AttachmentSlots`
- Keep old components in the schema, but stop adding them in `entity_makers.cpp` once stable.

**Immediate “small win” option (lower risk)**

Even before adding new components:

- Stop using `CanBeHeld_HT` for new handtrucks; use `CanBeHeld`.
- Add a one-time migration: `CanBeHeld_HT` -> `CanBeHeld`.
- Update collision code to only check one “held flag”.

---

### 4) Replace machine-specific `HasWork` lambdas with a “machine” component + system

**Problem**

Many machines are implemented as:

- a furniture entity with `HasWork` bound to a lambda in `entity_makers.cpp`
- plus separate ad-hoc systems that do similar things (e.g., soda fountain auto-adds soda)

This creates duplication and makes behavior harder to audit.

**Proposal**

Create a small set of reusable machine components + systems, e.g.:

- **`DispensesIngredient`**:
  - ingredient kind (static ingredient, from an adjacent item, from a held item with `AddsIngredient`, etc.)
  - rules (`require_empty`, `allow_multiple`, `must_be_held_by X`, etc.)
  - rate/progress configuration and sound id
- **`MachineWork`** (optional): standardized progress + completion handling (could reuse `HasWork` internally, or replace it over time)

Then implement:

- `IngredientDispenserSystem` for draft tap, ice machine, soda fountain (and later squirter).

**Near-term target**

- Fix the **draft tap duplication** by making beer-tap ingredient addition come from one shared implementation.

---

### 5) Consolidate customer AI state into a single `CustomerAI` component

**Problem**

AI state is split across:

- `CanPerformJob` (job enum)
- a component per job/behavior (`AIWaitInQueue`, `AIDrinking`, `AIUseBathroom`, `AIPlayJukebox`, `AICloseTab`, `AIWandering`, `AICleanVomit`)

These components mostly exist to hold:

- cooldown timers
- a target id + “find target” logic
- line-wait state

This creates:

- many components to add/remove/reset
- repeated “find target” patterns across different AI behaviors
- the need for `reset_job_component<T>()` component churn

**Proposal**

Add **one** component for customers:

- **`CustomerAI`**:
  - current target ids (register/toilet/jukebox/target-location)
  - per-behavior timers/cooldowns
  - queue state (line position, last position, etc.)
  - “next job after X” state (wandering/bathroom)

Then:

- Keep `CanPerformJob` as the job selector.
- Replace the per-job components with `CustomerAI` sub-state (structs inside the component).
- Replace `reset_job_component<T>()` with `customer_ai.reset(JobType)`.

**Migration approach (schema-safe)**

- Append `CustomerAI` to snapshot schema.
- Add a migration system that:
  - if a customer has any legacy AI components, populate `CustomerAI` (best-effort) and then the AI system reads only `CustomerAI` going forward.
- Stop attaching the per-job AI components for newly created customers once stable.

---

### 6) Make “should_run gating” a shared utility (remove boilerplate + try/catch)

**Problem**

Many systems duplicate this pattern:

- check `GameState::get().is_game_like()`
- try/catch around `Sophie` lookup
- check `HasDayNightTimer.needs_to_process_change`
- check `bar_open` or `bar_closed`

This is noisy and makes it easy to introduce subtle differences.

**Proposal**

Create a small shared helper (e.g. `SystemRunContext`) that is computed once per tick:

- `bool game_like`
- `bool model_test`
- `bool in_transition`
- `bool bar_open`
- pointers/refs to `Sophie` and her key components when available

Then systems do:

- `if (!ctx.inround()) return false;`
- `if (ctx.transition()) return false;`

This shrinks code and makes state logic consistent.

## Component merge/rename opportunities (shortlist)

This section lists “candidate simplifications” with suggested end-states.

### Holding / interaction

- **Merge**: `CanBeHeld` + `CanBeHeld_HT` -> one `Holdable` (new) or one canonical “held” concept.
- **Consolidate**: `CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck` -> `AttachmentSlots`.
- **Consider merge**: `CanBeHighlighted` + `CanHighlightOthers` into an “Interaction” model:
  - `Interactor` (reach, current target id)
  - `Interactable` (highlight rules, work rules)
  - This enables avoiding “clear highlight on everything every frame”.

### Variants/index selection

- **Unify semantics**: `Indexer` and `HasSubtype` are both “index selection”.
  - Long-term: one `VariantIndex` component with:
    - range
    - value
    - last_rendered_value (for change detection)

### Store flags

- Consider combining `IsFreeInStore` + `IsStoreSpawned` into one `StoreItem` component:
  - `bool spawned_from_store`
  - `bool free`
  - optional `price_override`

### Names / localization

`HasName` is used for both debug and user-facing (with TODOs suggesting i18n). Consider:

- `HasDisplayName` (TranslatableString / i18n id + params)
- `HasDebugName` (plain string)

## Entity factory simplification (`entity_makers.cpp`)

**Problem**

Factory logic is long and mixes:

- base entity setup
- renderer decisions
- behavior wiring via lambdas
- conversion logic (`convert_to_type`)

**Proposal**

1) Replace `convert_to_type` switch with a registry:

- `std::array<FactoryFn, EntityType::COUNT>` where missing entries are obvious
- this makes “unhandled types” more explicit and helps avoid accidental fallthrough

2) Extract repeated “base furniture” / “base item” patterns into small reusable helpers.

3) Move “machine behavior” out of factories into systems via machine components (see above).

## Phased execution plan (safe, incremental)

### Phase 0 — Inventory + guardrails (1–2 PRs)

- Add a doc/table enumerating:
  - components in `snapshot_blob::ComponentTypes`
  - which entity types use which components (starting from `entity_makers.cpp`)
  - which systems read/write each component
- Add lightweight assertions/logging for:
  - “Sophie exists” assumptions
  - “container config” assumptions

### Phase 1 — Low-risk system consolidation (no schema changes)

- Implement `TriggerAreaSystem` consolidation.
- Implement `ContainerMaintenanceSystem` consolidation (including ModelTest path).
- Introduce `SystemRunContext` and refactor repeated should_run patterns.

### Phase 2 — Holding model unification (schema changes, migration-based)

- Add new components (`Holdable`, `AttachmentSlots`) by **appending** to schema.
- Add migration system(s) to translate legacy held/holding data.
- Replace 3 separate “held position” systems with one `UpdateAttachmentsSystem`.
- Update input/collision code to use the unified concept.

### Phase 3 — “Machine” refactor (progressive, keeps gameplay identical)

- Add `DispensesIngredient` and `IngredientDispenserSystem`.
- Migrate:
  - soda fountain
  - ice machine
  - draft tap
- Remove duplicated beer-tap logic inside drink `HasWork`.

### Phase 4 — AI consolidation (largest simplification, migration-based)

- Append `CustomerAI` to schema.
- Add migration that seeds `CustomerAI` from legacy per-job components.
- Update AI processing to use `CustomerAI` exclusively.
- Stop attaching legacy AI components for newly created customers.

### Phase 5 — Factory registry + cleanup

- Replace `convert_to_type` giant switch with a registry map/array.
- Move remaining “lambda behaviors” into explicit systems where appropriate.

## “If we only do three things” recommendation

1) **TriggerAreaSystem consolidation** (big clarity gain, no schema risk)
2) **ContainerMaintenanceSystem consolidation** (removes duplicated logic + state differences)
3) **DraftTap / drink ingredient logic de-dup** via a shared helper or `DispensesIngredient`

These three alone should noticeably simplify the architecture without a large migration burden.

