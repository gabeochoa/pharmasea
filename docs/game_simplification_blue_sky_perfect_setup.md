# Blue-sky “Perfect” Architecture (Breaking Changes Allowed)

This is an intentionally **idealized** target architecture for the game if we’re allowed to make **breaking changes** (rename/remove components, redesign snapshots, restructure systems, and refactor factories freely).

The north star is: **few concepts, few components, few systems, clear data flow**.

## Core philosophy

- **Components are data only** (no callbacks, no lambdas, no embedded behavior).
- **Systems own behavior** and operate on small, coherent “domains” (movement, interaction, economy, AI, etc.).
- **Game rules are data-driven** via “archetype definitions” (JSON/TOML/YAML/C++ tables).
- **No giant switches** for entity creation or game-state branching.
- **One interaction model** instead of multiple ad-hoc hold/highlight/work pipelines.

---

## Perfect ECS surface area (minimal component set)

### A) Always-present primitives

- `Transform`: position, rotation/facing, scale, optional visual offset.
- `Tags`: entity type/category flags (player/customer/furniture/item/etc.).

### B) Rendering (pick one model)

Option 1 (simple, flexible):
- `RenderModel`: model id + variant key (e.g. “cup_ingredients=…”).
- `RenderTint` (optional): for debug/placeholder visuals.

Option 2 (more “engine-y”):
- `Renderable`: handle to render proxy, updated by system.

### C) Physics / collision

- `Collider`: shape + layer + collision mask.
- `KinematicBody`: velocity, desired movement (optional).

**Remove**: “collision rules embedded in input code” and “special case held objects” logic scattered around.

### D) Interaction / attachments (replace the whole holding ecosystem)

This is the big simplifier.

- `Interactor` (on players and possibly AI):
  - reach radius
  - current focus entity id (what I’m looking at / best candidate)
  - optional “interaction intent” state (grab/place/use)

- `Interactable` (on objects that can be targeted):
  - interaction kind(s): `PickUp`, `PlaceInto`, `Work`, `Toggle`, `Use`, `Buy`, etc.
  - optional requirements (filters)

- `Inventory` (for “held items”):
  - a small list of slots (hand slot, tool slot, carried slot)
  - each slot holds an entity id

- `Attachment` (on held/attached entities):
  - parent entity id
  - socket id / rule
  - local offset rule

This replaces:
- `CanHoldItem`, `CanHoldFurniture`, `CanHoldHandTruck`
- `IsItem` “held by” semantics
- `CanBeHeld`, `CanBeHeld_HT`
- multiple “update held position” systems

### E) Work / progress

Replace `HasWork` callbacks with pure data:

- `Workable`:
  - progress \([0..1]\)
  - work rate scaling (base rate, tool multiplier)
  - “work action id” (data key), not a lambda
  - progress UI flags

Then behavior is in systems keyed by `work_action_id`.

### F) Machines / crafting / recipes

