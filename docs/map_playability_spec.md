# Map Playability Spec (Start-of-Day / Day-1)

This document is the **single source of truth** for whether a generated map is
acceptable to start a new day (especially Day-1).

It is intentionally **generator-agnostic**: any generator that outputs an ASCII
grid (the “ASCII seam”) must satisfy this spec.

See also: `map_generation_plan.md` for roadmap + rationale.

## Definitions

- **ASCII grid**: `std::vector<std::string>` where each character is a tile.
- **Walkable (ASCII-level)**: any tile that is not an outside-facing wall tile
  (`#` or `w`). (Runtime walkability is authoritative, but this definition is
  sufficient for generation-time structural checks.)
- **Inside BAR_BUILDING**: the runtime rectangle in `src/building_locations.h`
  used by Sophie to filter valid registers.

## Hard requirements (must hold on first frame of Day-1)

### Required entities exist

The ASCII must contain at least one of each:

- `C`: CustomerSpawner (**must be outside** the bar interior)
- `R`: Register (**at least one** must be inside `BAR_BUILDING`)
- `S`: SodaMachine
- `d`: Cupboard (cups)
- `g`: Trash
- `f`: FastForward
- `+`: Sophie
- `t`: Table (**≥ 1 total**)

### Spawner → Register must be pathable (with engine cutoff)

There must exist at least one register such that:

- Customers can path from the spawner to the register’s
  `Transform::tile_directly_infront()`.
- The BFS pathfinder’s hard cutoff applies: `bfs::MAX_PATH_LENGTH == 50`
  (distance-squared gate). Treat exceeding this as **invalid**.

### Register queue strip must be clear

For at least one register that Sophie considers valid (inside `BAR_BUILDING`):

- All tiles in the queue strip are walkable:
  `Transform::tile_infront(1..HasWaitingQueue::max_queue_size)`.
- Current queue length: `HasWaitingQueue::max_queue_size == 3`.

### Connectivity (no diagonal-only connections)

The walkable region relevant to gameplay must be connected under **4-neighbor**
adjacency (N/S/E/W). Diagonal-only adjacency does not count.

### Shell / footprint stability

- The bar footprint is ~**20×20** for MVP.
- The outside-facing shell (outer walls / silhouette) is stable over time.

## Protected tiles (generation-time constraints)

Certain tiles must remain walkable and unobstructed during map generation. These
"protected tiles" should not have furniture, decorations, or obstacles placed on
them by the decoration stage or any auto-placement system.

### Queue strip tiles

The tiles directly in front of the register where customers queue:

- Positions: `register_tile + facing * (1..max_queue_size)`
- Current queue length: **3 tiles** (`HasWaitingQueue::max_queue_size`)
- These tiles must be walkable floor (`.`) at generation time

### Entrance throat tiles

The opening(s) in the outer wall that connect outside to inside:

- Width: **1–3 tiles** per entrance
- Must connect to a walkable path leading to the register queue

### Critical path corridor

The shortest path from `CustomerSpawner` to the register queue front:

- All tiles on this path must remain walkable
- Decoration should avoid placing obstacles on or immediately adjacent to this path
- The path must stay under `bfs::MAX_PATH_LENGTH` (50 tiles) distance

### Implementation note

The placement stage in `day1_required_placement.cpp` tracks protected tiles in a
`blocked` bitmap and excludes them when placing other required entities:

```cpp
// Protect queue strip + found path
std::vector<std::vector<bool>> blocked(h, std::vector<bool>(w, false));
for (int dj = 1; dj <= HasWaitingQueue::max_queue_size; dj++) {
    blocked[reg.i][reg.j + dj] = true;
}
for (const grid::Cell& p : path) {
    blocked[p.i][p.j] = true;
}
```

## Failure taxonomy + deterministic retry policy

### Outcomes

- **Repairable**: can be fixed by a small local edit without changing the overall
  archetype (e.g., rotate/move a register to clear its queue strip).
- **Reroll-only**: structural issues where repair is likely to cascade (e.g.,
  disconnected walkable regions).

### Retry policy (deterministic)

- Use `(seed, attempt_index)` deterministically, e.g. derive RNG seed as
  `hash(seed + ":" + attempt_index)`.
- Default retry cap: **25 attempts**.
- If all attempts fail, surface a clear reason code (first hard failure) so the
  player/dev can reroll or adjust generator knobs.

## Coordinate convention

### Grid to world mapping

- `grid[i][j]` corresponds to world position `(x = i * TILESIZE, y = j * TILESIZE)`
- Row index `i` = world X axis
- Column index `j` = world Y/Z axis

### Origin marker

- The `0` character marks the origin tile in ASCII
- After offset, this tile should map to world `(0, 0)`
- **Known quirk**: `find_origin()` in `generation::helper` uses swapped indices;
  treat this as tech debt to resolve in future cleanup

### "Right wall" entrance

- "Entrance on the right (+x) wall" = **last row** of ASCII grid (not last column)
- The entrance gap connects outside walkable area to inside

## Seed determinism

### Stage-specific seeds

Each pipeline stage derives its RNG seed from the base seed:

| Stage | Seed derivation | Example |
|-------|-----------------|---------|
| Layout | `hash(seed + ":layout:" + attempt)` | `"alpha:layout:0"` |
| Placement | `hash(seed + ":place:" + attempt)` | `"alpha:place:0"` |
| Decoration | `hash(seed + ":decor:" + attempt)` | `"alpha:decor:0"` |

### Attempt index behavior

- The `attempt_index` increments on layout/placement failures (rerolls)
- Same `(seed, attempt_index)` always produces the same output
- Max attempts: **25** (`DEFAULT_REROLL_ATTEMPTS`)

### Cross-platform determinism

- RNG uses `std::mt19937` seeded via `hashString()`
- Layout algorithms should avoid floating-point operations that vary by platform
- WFC may have platform-specific behavior; fallback to Simple layout is acceptable

