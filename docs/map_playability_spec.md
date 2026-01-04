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

