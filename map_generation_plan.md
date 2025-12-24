# Map Generation Plan (PharmaSea)

This document is a **planning note only**. It outlines a roadmap to update map generation without prescribing immediate code changes.

## Context (current architecture)

- **ASCII seam**: Map generators ultimately produce a `std::vector<std::string>` “tile map”. A single step (ASCII → entities) spawns entities and then validates.
- **Generators available**:
  - **Room/region generator** (currently wired for gameplay): produces a `std::vector<char>` and is converted into ASCII lines.
  - **WFC (Wave Function Collapse)** (currently more standalone/testing): driven by `resources/config/map_generator_input.json`.
- **Trigger point**: In-game map-gen is triggered from gameplay (`LevelInfo::generate_in_game_map()`), which delegates to the in-game map generation entry points.

## Hard constraints (from design requirements)

- **Day-1 must be playable**, i.e. the starting map must support the basic serving loop.
  - **Cups** are always provided by **Cupboard**.
  - **Customers spawn outside**: a `CustomerSpawner` must exist outside, and there must be a path to **at least one Register**.
- **Footprint is fixed**: currently ~**20×20**.
- **Outside-facing walls remain the same over time**: the “shell” should not change as progression advances.
- **Variety**: generate different bar “types” (one giant room, multiple rooms, back-room/speakeasy vibe).
  - “Speakeasy” is **visual only** (not locked; still pathable).
- **Prefer openness / capacity**: leave room for late-game content (teleporters, conveyors, more machines/ingredients).

## Code-informed MVP constraints (what the game enforces today)

This section captures constraints observed directly in the current code so the
next generator matches real gameplay assumptions.

### Required entities for a playable start (MVP)

From your requirements + current in-game generator defaults, the starting bar
should include:

- `CustomerSpawner`
- Exactly **one** `Register`
- `SodaMachine` (this matches the current day-1 assumption you confirmed)
- `Cupboard` (cups)
- `Trash`
- `FastForward`
- `Sophie` (must exist, location not gameplay-critical)
- At least **one** `Table`

### Register queue rule (must not be blocked)

Sophie’s lightweight map validation checks queue tiles in front of each register:

- Queue length is `HasWaitingQueue::max_queue_size` (**currently 3**).
- For each register, it requires all positions `Transform::tile_infront(i+1)`
  to be walkable for `i ∈ [0..max_queue_size-1]`.
- It also checks pathability from `CustomerSpawner` → `Register::tile_directly_infront()`.

Therefore, placement should:

- Pick a register tile, then **try 4 facings/rotations** and choose a facing
  where the full queue strip is clear.

### Register must be “inside BAR_BUILDING” (keep as-is for MVP)

Sophie filters valid registers using:

- `whereInside(BAR_BUILDING.min(), BAR_BUILDING.max())`

`BAR_BUILDING` is currently a fixed rectangle in `src/building_locations.h`.
For MVP we should place the register inside that rectangle to avoid runtime
`REGISTER_NOT_INSIDE` failures even if the register is “in the plot”.

### Pathfinding has a hard distance cutoff (treat as hard failure)

The BFS-based pathfinder has a built-in maximum range:

- `bfs::MAX_PATH_LENGTH = 50` (used as a distance-squared cutoff)

So if spawner→register would require more than ~50 tiles of travel, pathing may
fail. Treat this as a **hard invalid map** in generation/validation for now.

### Coordinate / “right wall” interpretation

The ASCII grid is mapped into world space such that:

- Row index `i` maps to world **X** (`x = i * TILESIZE`)
- Column index `j` maps to world **Y/Z** (`y = j * TILESIZE`)

So “entrance on the right (+x) wall” corresponds to the **last row** of the
ASCII building boundary, not the last column.

## Current DEFAULT_MAP notes (entrance + symbols)

`DEFAULT_MAP` is loaded from `resources/config/settings.json`.

- It includes `S` (SodaMachine), `d` (Cupboard), `g` (Trash), `f` (FastForward),
  `R` (Register), `C` (CustomerSpawner), `+` (Sophie), and many `t` (Tables).
- The building boundary uses `#` walls with an “entrance gap” near the bottom.
- In the current `DEFAULT_MAP`, the main gap appears in the second-to-last wall
  row as:
  - `######...########`
  which is a **3-tile-wide** opening leading into the outside walkable row
  beneath it (`.....f.........C+`).

For MVP you indicated the entrance can be 1–2 tiles wide; consider keeping this
flexible (1–3) initially so existing expectations remain easy to match.

## Additional clarified requirements (from Q&A)

- **Entrance width**: allow **1–3 tiles** (to match current `DEFAULT_MAP`).
- **Entrance location**: randomize along the right (+x) wall, as long as
  spawner→register pathing succeeds under the 50-tile cap.
- **Multiple entrances**: sometimes allow more than one entrance (suggested
  starting probability: **~10%** of runs), still respecting pathing/queue rules.
- **Interior wall thickness**: **1 tile**.
- **No diagonal-only connectivity**: treat diagonal-only connections as invalid
  (players can’t traverse them). Generation and validation should ensure the
  walkable interior is connected under **4-neighbor** adjacency, even if the
  current BFS considers 8 neighbors.
- **Table density**: soft target of **~2 tables per room** (with a floor of ≥1
  table total).
- **Wall density / openness**: avoid too many interior walls because players
  can’t move walls. Prefer open layouts with a configurable openness target
  (currently **~70%** walkable interior).
- **Reroll behavior**: silently retry failed generations up to a configurable
  cap (deterministic seed + attempt index), then fail/ask for reroll.

## Guiding principle

Keep **ASCII (`std::vector<std::string>`) as the intermediate representation**:

1) Generator outputs ASCII lines (layout + possibly tags)  
2) A single “instantiate + validate” step turns ASCII into entities  

This keeps gameplay insulated from generator changes.

## Target pipeline (generator-agnostic)

### 1) Layout

Generate the walkable/blocked structure.

- Keep a **fixed shell** (outer walls + outside zone shape).
- Use an **archetype-first** approach: pick a bar style, then generate to match it.
- Maintain a tunable **openness/capacity** target (walkable ratio inside).

### 2) Required placement (hard constraints for start-of-day maps)

Place required stations after layout so they land in sensible locations.

- Place `CustomerSpawner` **outside**.
- Place at least one `Register` so that:
  - It’s reachable from the spawner (path exists), and
  - Its queue tiles are valid (i.e., the “in front of register” queue strip is walkable).
- Place other day-1 required stations (Cupboard, soda capability, at least one table) based on your progression rules.

### 3) Decoration (optional)

Add props/variation without breaking critical paths.

- Prefer sparse or clustered decoration.
- Avoid blocking queue tiles or critical routes (entrance/spawner ↔ register).

### 4) Validation + deterministic repair/retry

Validate the ASCII map (and/or instantiated entities) against a playability spec.

- Prefer **deterministic retries**: same seed + attempt index (bounded tries).
- Use **repair** for local issues (e.g., blocked queue tile), **reroll** for structural issues (e.g., disconnected interior).

## Aligning validation with “Sophie” / runtime realities

Players can move items during the game, so **runtime checks** will still matter.

For map generation specifically, the goal is:

- Generation-time validation should ensure the **starting map** does not immediately violate the same structural constraints that gameplay checks will complain about (notably: spawner→register pathing and register queue tile availability).
- Document a single **Playability Spec** listing which conditions must hold:
  - At **start-of-day** (generation guarantee)
  - At **phase transitions / runtime** (UI feedback / blockers)

This avoids divergence where generation says “valid” but runtime checks immediately disagree.

## Bar styles (archetypes) to support

- **Open hall**: one large interior area, minimal partitions.
- **Multi-room**: 2–5 rooms connected by short corridors/doorways.
- **Back-room vibe**: front area + back room connected by a single narrow connection; visually “in the back” but still accessible.
- **Loop/ring**: interior circulation loop for interesting movement and robust pathing.

## Milestones

### 1) Document the Playability Spec

Produce a concise spec (bulleted rules) covering:

- Required entities for day-1 (and how that evolves with progression)
- Spawner outside requirement
- Spawner → register path requirement
- Register queue tile rules (walkability strip in front of register)
- Fixed shell constraints (20×20, outside wall ring consistent)

### 2) Add archetype selection (no generator swap yet)

Add a concept of “bar style” selection (weighted/random by seed), even if it initially maps to variations of the current generator.

### 3) Split generation into explicit stages

Refactor conceptually into:

- layout → required placement → decoration → validate → repair/retry

Keep ASCII between stages.

### 4) Bring WFC under the same interface

Treat WFC as an alternative **layout source** that still feeds the same placement + validation pipeline.

### 5) Add deterministic regression coverage

Create a seed list and basic properties:

- For each seed, generation produces a valid start-of-day map
- Connectivity and required counts hold
- No immediate “invalid queue tiles” cases

## 6) Investigate new map generators (choose a short-list)

### Evaluation criteria (decide before comparing)

- **Hard guarantees**: can it reliably satisfy spawner outside + path to a register + queue tile constraints?
- **Variety**: can it produce the desired bar types (open/multi-room/back-room vibe)?
- **Openness/capacity controls**: can we tune walkable ratio and keep usable placement space?
- **Determinism**: stable outputs from seed (across platforms/compilers if needed).
- **Iteration/debuggability**: can failures be diagnosed and fixed quickly?

### Candidate approaches to evaluate

- **Room-graph / BSP + rasterization**:
  - Strong for “bar archetypes”, connectivity guarantees, and fixed footprint.
  - Usually easiest to keep layouts open while still varied.
- **Constraint-based layout + placer**:
  - Generate mostly-open floorplan, then apply constraints for partitions/features.
  - Robust on small maps (20×20) and good for deterministic repair.
- **Prefab/template stitching (macro-tiles/chunks)**:
  - Great for authored “speakeasy/backroom” vibes and recognizable structure.
  - Deterministic, fast, and easy to debug; variety comes from mixing templates.
- **Post-processed random walks / maze variants**:
  - Can produce interesting silhouettes but often needs careful openness controls and connectivity validation.
- **WFC (existing)**:
  - High variety, but pattern sets can overconstrain and increase failure rates.
  - Best positioned as **layout-only** feeding shared placement + validation stages.

### Deliverable for step 6

A short decision note selecting:

- 1 default generator to implement next
- 1 secondary generator to keep as an alternate (optional)
- Rationale mapped to the evaluation criteria above

## 7) Implement the chosen generator (behind the pipeline)

### Implementation constraints

- Preserve the **ASCII seam** and keep “instantiate + validate” as a single downstream step.
- Plug into in-game map generation via a **small generator interface**:
  - `generate(seed, context) -> std::vector<std::string>`
- Use deterministic repair/retry so **day-1 is always playable**.

### Recommended implementation order

- Implement the new generator as **layout-only** first.
- Reuse a shared **required placement** stage to satisfy hard constraints.
- Reuse a shared **validation + retry** stage.
- Add archetype tuning knobs (room count, back-room chance, openness target).

### Definition of done

- A set of fixed seeds consistently produces valid day-1 maps.
- Multiple archetypes appear across seeds (visible structural variety).
- Layout preserves enough open space to accommodate late-game machines/items.

