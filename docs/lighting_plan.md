# Lighting Plan (Day/Night + Bar Spill + Outdoor Sun Shadows)

## Ideas

### Visual goals

- **Night should read as night**
  - Outdoors is noticeably darker/cooler.
  - The bar casts warm light that spills outside (door/windows).
  - Indoors stays readable (fixtures/ambient), not sunlit.
- **Day should read as day**
  - Outdoors is brighter/warmer with a sun direction cue.
  - **Sun shadows outdoors only** (roofs exist “in real life” even if not rendered).
  - Indoors should not receive direct sun shadows.

### Constraints that shape the solution

- The current world rendering is mostly **unlit** (flat colors + `DrawModelEx(..., WHITE)` and `DrawCubeCustom(...)`), so “real 3D lighting everywhere” is a bigger renderer change than a feature toggle.
- The game already renders into a **render texture** and optionally applies post-processing. That makes it easy to insert a world-only lighting pass without touching UI.
- “Roofs aren’t rendered, but exist” implies we need an **explicit indoor/outdoor mask** regardless of the lighting technique.

### Lighting approaches (high-level)

- **Option A (start)**: screen-space lighting overlay
  - Apply night/day ambience via fullscreen tint/multiply.
  - Add local lights via additive shapes (cone/circles).
  - Add outdoor-only shadows via projected polygons + indoor masking.
  - Best for speed and for enforcing “outdoor-only” constraints.
- **Option B (later)**: render a `lightRT` and composite in a shader
  - Cleaner pipeline and easier softening/blur, but more plumbing.

### Raylib-style “real” lighting (what it would look like here)

- Raylib lighting is typically **shader-based**: you assign a lighting shader to `Model` materials and feed light uniforms per-frame.
- This is **easy-ish for models** (characters/props) and **harder for your cubes**, because most cubes are drawn via `rlgl` immediate-mode (`DrawCubeCustom`) and don’t share a material/shader pipeline.
- Even with real lighting, we still need a roof/indoor mask to enforce “outdoor-only sun/shadows”.

### Static geometry batching idea (convert cubes → meshes, merge for draw-call reduction)

The buildings/walls “never change after spawn”, so we can convert their cube geometry into one or more **static `Mesh`/`Model` batches** built once per map generation:

- **What to batch first**
  - `EntityType::Wall` is the best first candidate (spawned by `generate_walls_for_building(...)`).
- **How batching works**
  - Keep wall entities for collision/pathing/gameplay, but mark them “no-render” (or remove their render component).
  - Build a `Mesh` that contains triangles for all wall cubes in a building (or a chunk).
  - Render that `Mesh` with a single draw call per batch.
- **Why chunking matters**
  - A single giant mesh hurts culling and can hit 16-bit index limits; per-building or per-16×16-tile chunk is a good default.
- **Optional next optimization**
  - “Greedy meshing” / internal-face culling can massively reduce triangles and vertex counts.
- **Lighting benefit**
  - Once cubes are meshes with normals, they can participate in shader lighting more naturally (if we want to go that direction later).

---

## Phases (actual changes needed)

### Phase 0 — Instrumentation & debug (so we can see what’s happening)

- **Add debug toggles**
  - Force-enable lighting even during day.
  - Show overlay-only view.
  - Draw debug gizmos for light origin/direction and indoor roof masks.
- **Where**
  - `GameLayer::onDraw()` is the correct place to keep “world lit, UI unlit”.

### Phase 1 — Option A baseline ambience (night/day + indoor/outdoor)

- **Add a world lighting overlay pass**
  - After `draw_world(dt)` and before `onDrawUI(dt)`.
- **Ambient**
  - Night: multiply or alpha-tint to darken the world.
  - Day: no-op or subtle warm tint.
- **Indoor mask**
  - Use `Building::area` rectangles as “roofed” zones.
  - Project roof rectangles to screen and lift indoors (additive) so interiors remain readable at night.

### Phase 2 — Option A night local lights (bar spill first)

- **Bar spill cone**
  - Use a stable bar entrance origin (either building door metadata or a door entity transform).
  - Draw an additive cone fan in screen space (layered cones for softness).
  - Clamp to outdoors (skip cone samples inside roof rectangles).
- **Optional**
  - Window/fixture glows (small additive circles/fans).

### Phase 3 — Option A day sun + outdoor-only shadows

- **Sun direction**
  - Start fixed (top-left) and optionally vary slightly with time.
- **Roof/building shadows**
  - Treat roofed rectangles as occluders and draw projected shadow polygons onto the ground.
  - Apply shadows only outdoors by masking/skipping triangles that land indoors.
- **Softening**
  - Fake penumbra by layering a few expanded/offset shadow polygons with lower alpha.

### Phase 4 — Static geometry batching v1 (convert wall cubes → static meshes)

- **Scope**
  - Batch only walls/building shells first (e.g., `EntityType::Wall`).
- **Mesh builder**
  - Add a `StaticGeometryBatcher` (new module) that:
    - scans entities after map generation,
    - groups eligible static cubes (by building or by chunk),
    - builds one or more raylib `Mesh` objects (with per-vertex color),
    - stores the resulting `Model`/`Mesh` handles in `LevelInfo` (not serialized; rebuild on load).
- **Render integration**
  - In `LevelInfo::onDraw(dt)`:
    - draw static mesh batches first,
    - render remaining dynamic entities normally.
- **Prevent double-render**
  - For batched entities:
    - add a “no-render” flag/component, OR
    - remove `SimpleColoredBoxRenderer` after batching, leaving collision/pathing intact.
- **Chunking**
  - Default to per-building meshes or spatial chunk meshes (e.g., 16×16) to keep culling effective and avoid index limits.

### Phase 5 — Static geometry batching v2 (reduce triangles + improve culling)

- **Internal face culling**
  - Skip cube faces that are adjacent to another cube in the batch.
- **Greedy meshing (optional)**
  - Merge coplanar faces into larger quads to reduce vertex/triangle count further.
- **Culling**
  - Keep chunk bounds for frustum culling.

### Phase 6 — Optional: shader-based 3D lighting for meshes/models (raylib-style)

- **Models**
  - Apply a lighting shader to model materials; feed uniforms for sun + bar light.
- **Static batched meshes**
  - Apply the same shader to the batched environment mesh material.
- **Keep the roof rule**
  - Continue using the indoor/outdoor mask (Option A or a lightRT) so “sun shadows outdoors only” still holds even if roofs aren’t rendered.


