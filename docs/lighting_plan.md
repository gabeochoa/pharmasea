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

- **Option A (start, shader-based)**: real lighting (Half-Lambert + Blinn-Phong)
  - Use a lighting shader for **models + cube geometry** (requires normals).
  - “Sun” as **directional** (recommended) or **point** (stylized).
  - Indoor/outdoor is enforced via a **roof rectangle mask** in the shader (disable direct sun indoors).
  - Shadows can come later (cheap projected shadows or shadow maps).
- **Option B (later)**: hybrid / compositing
  - Optional `lightRT` + post composite for additional stylization (vignette, bloom, fog, night grading).

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

**Replace overlay lighting with shader lighting**:

- **Add lighting shaders**
  - `resources/shaders/lighting.vs` / `resources/shaders/lighting.fs`
  - Implement Half-Lambert diffuse + Blinn-Phong specular.
- **Ensure normals exist**
  - Update `DrawCubeCustom(...)` to submit per-face normals via `rlNormal3f(...)`.
  - Ensure models use normals (most do).
- **Apply shader to geometry**
  - Wrap the 3D world draw in `BeginShaderMode(lightingShader)` so immediate-mode cubes/planes are lit.
  - Assign `lightingShader` to each model material so `DrawModelEx(...)` is lit too.
- **Sun + ambient uniforms**
  - Set `viewPos`, `ambientColor`, `lightType`, `lightPos`/`lightDir`, `lightColor`, `shininess`, `useHalfLambert` each frame.
- **Indoor/outdoor rule (“roof exists”)**
  - Pass `roofRects[]` (minX/minZ/maxX/maxZ) into the shader.
  - In fragment, disable **direct sun** indoors (keep ambient).

### Phase 2 — Option A night local lights (bar spill first)

- **Add bar lights as additional shader lights**
  - Add 1–N point lights near the bar entrance/window fixtures.
  - Pass as an array of light structs/uniform arrays (position, color, radius, enabled).
  - Apply only during night, and optionally only outdoors (or attenuate indoors).

### Phase 3 — Option A day sun + outdoor-only shadows

- **Sun direction/position**
  - Prefer directional sun; optionally keep point-sun for stylization.
- **Shadows (choose one)**
  - **Cheap**: project roof/building shadows onto ground using simple geometry and blend (works well with “invisible roofs”).
  - **Real**: shadow mapping (extra render pass + shader sampling + bias tuning).

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

Folded into Phase 1 now (Phase 6 becomes “polish and expand light types/shadows”).


