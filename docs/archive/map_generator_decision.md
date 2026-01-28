# Map Generator Decision Note

This document evaluates candidate layout generators against the criteria defined
in `map_generation_plan.md` Phase 5 and recommends the next generator to implement.

## Evaluation Criteria (from plan)

1. **Hard guarantees**: Can it reliably satisfy spawner outside + path to register + queue tile constraints?
2. **Variety**: Can it produce the desired bar types (open/multi-room/back-room vibe)?
3. **Openness/capacity controls**: Can we tune walkable ratio and keep usable placement space?
4. **Determinism**: Stable outputs from seed (across platforms/compilers)?
5. **Iteration/debuggability**: Can failures be diagnosed and fixed quickly?

## Current State

Two layout providers exist today:

| Provider | Status | Strengths | Weaknesses |
|----------|--------|-----------|------------|
| Simple (room-region) | Default | Fast, deterministic, good openness | Limited variety, basic archetypes |
| WFC | Available | High variety potential | Higher failure rate, harder to debug |

Both feed into the shared placement + validation pipeline, so playability is enforced regardless of layout source.

## Candidate Approaches

### 1. Room-graph / BSP + Rasterization

**Description**: Generate a graph of rooms with connections, then rasterize to ASCII.

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Hard guarantees | ★★★★★ | Connectivity is structural; easy to ensure paths exist |
| Variety | ★★★★☆ | Good for open/multi-room; back-room needs extra logic |
| Openness control | ★★★★★ | Direct control over room sizes and wall density |
| Determinism | ★★★★★ | Integer-only operations, fully deterministic |
| Debuggability | ★★★★★ | Room graph is easy to visualize and trace |

**Pros**: Strong fit for archetypes, easy to add "required room" concepts (e.g., always have a main hall + back room option).

**Cons**: May feel "boxy" without additional polish passes.

### 2. Constraint-based Layout + Placer

**Description**: Start with mostly-open floor, apply constraints to add partitions/features.

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Hard guarantees | ★★★★☆ | Constraints can enforce paths but may need repair passes |
| Variety | ★★★☆☆ | Variety comes from constraint variations |
| Openness control | ★★★★★ | Start open, only add walls where needed |
| Determinism | ★★★★★ | Constraint solvers are typically deterministic |
| Debuggability | ★★★★☆ | Constraint failures can be harder to trace |

**Pros**: Naturally open layouts, good for late-game expansion space.

**Cons**: Less "authored" feel; may need more tuning to get distinct archetypes.

### 3. Prefab/Template Stitching

**Description**: Author small templates (2-5 tile chunks), stitch them together.

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Hard guarantees | ★★★★★ | Templates are pre-validated; stitching rules ensure connectivity |
| Variety | ★★★★★ | Easy to add new templates for specific vibes |
| Openness control | ★★★★☆ | Depends on template design |
| Determinism | ★★★★★ | Template selection is seed-based |
| Debuggability | ★★★★★ | Templates are human-authored and easy to inspect |

**Pros**: Great for "speakeasy/backroom" authored vibes, fast execution, easy to add new content.

**Cons**: Requires up-front template authoring; less emergent variety.

### 4. WFC (existing)

**Description**: Wave Function Collapse using pattern sets from config.

| Criterion | Rating | Notes |
|-----------|--------|-------|
| Hard guarantees | ★★☆☆☆ | Pattern sets can overconstrain; high failure rate possible |
| Variety | ★★★★★ | Excellent emergent variety |
| Openness control | ★★☆☆☆ | Hard to tune; depends on pattern set |
| Determinism | ★★★★☆ | Mostly deterministic but edge cases exist |
| Debuggability | ★★☆☆☆ | Failures are hard to trace to specific patterns |

**Pros**: Already implemented; high variety when it works.

**Cons**: Unreliable without careful pattern curation; fallback to Simple already exists.

## Recommendation

### Primary: Room-graph / BSP + Rasterization

**Rationale**:
- Strongest match for the archetype-first approach (OpenHall, MultiRoom, BackRoom, LoopRing)
- Connectivity is guaranteed by construction
- Easy to add "required room" concepts for progression (e.g., "backroom unlocks on Day 5")
- Integer-only operations ensure cross-platform determinism
- Room graph provides natural debugging visualization

**Implementation notes**:
1. Start with a simple BSP split for MultiRoom archetype
2. Add "main hall + side rooms" variant for OpenHall
3. Add "front + back connected by corridor" for BackRoom
4. Add "circulation loop" post-process for LoopRing
5. Rasterize to ASCII, feed into existing placement + validation pipeline

### Secondary: Keep WFC as alternate

**Rationale**:
- Already implemented and integrated
- Provides variety when pattern sets are well-tuned
- Fallback logic already handles failures gracefully
- Can be used for "experimental" or "chaos" mode seeds

## Next Steps

1. ✅ Complete Phase 4 seed suite tests (done)
2. Implement Room-graph layout provider as `layout_roomgraph.cpp`
3. Add BSP room generation with archetype knobs
4. Wire into pipeline alongside Simple and WFC
5. Expand seed suite to cover new generator

## Decision Summary

| Role | Generator | Status |
|------|-----------|--------|
| **Default (new)** | Room-graph / BSP | To implement (Phase 6) |
| **Current default** | Simple | Keep as fallback |
| **Alternate** | WFC | Keep for variety |

