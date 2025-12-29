# Lighting Plan (Day/Night + Bar Spill + Outdoor Sun Shadows)

## Goals (what “good” looks like)

- **Night should read as night**:
  - Outside world is noticeably darker/cooler.
  - The bar/building emits warm light that spills onto the outside (near doors/windows).
  - Inside spaces can remain readable (not pitch black), but should feel “lit by fixtures” vs daylight.
- **Day should read as day**:
  - Outside is brighter/warmer, with visible sun direction.
  - **Sun shadows appear only outdoors** (roofs exist “in real life” even if not rendered).
  - Indoors should not receive direct sun shadows.

## Current state (what the codebase supports today)

- **Rendering is effectively unlit**:
  - Most world drawing uses solid colors (`DrawCubeCustom`, `DrawModelEx` with `WHITE`).
  - There’s no per-pixel lighting model or normals-driven shading in the current pipeline.
- **There is a render-to-texture + post-process stage**:
  - `App` renders all layers into `mainRT` each frame (`BeginTextureMode(mainRT)`).
  - `mainRT` is then drawn to screen with an optional post-processing shader (`post_processing.fs`).
- **Day/night state already exists**:
  - `HasDayNightTimer` controls day vs night and exposes `is_bar_open()` / `is_bar_closed()` and `pct()`.
  - Transition systems already fire triggers on day/night boundaries (`RespondsToDayNight`).
- **Buildings have simple world-space rectangles**:
  - `building_locations.h` defines `Building` areas for bar/store/etc and has `is_inside(vec2)`.
  - These rectangles can be treated as “roofed” regions to gate sunlight/shadows.

## Constraints & implications

- Adding “real” dynamic lights + shadow mapping for all geometry is a **big jump** (materials, normals, depth/shadow passes).
- The quickest path that fits current architecture is **screen-space lighting via a lightmask overlay**:
  - Works with unlit rendering.
  - Can still look great for top-down/isometric style.
  - Lets us cleanly enforce “outdoors-only sun/shadows” using building rectangles.

## Proposed approach (phased, lowest-risk first)

### Phase 0 — Decide the lighting pipeline shape

Two viable implementations; both can coexist with the current `mainRT` → post-processing workflow.

#### Option A (start here): Draw lighting directly onto `mainRT` after world render

This option treats lighting as a **2D overlay pass** rendered *after* the 3D world has already been drawn into the frame’s render target. It’s the least invasive path given the current “unlit” world rendering.

##### Where it runs (exact draw order)

The goal is: **world is affected, UI is not**.

- **World pass**: `GameLayer::draw_world(dt)` draws all 3D content.
- **Lighting pass (new)**: immediately after `draw_world(dt)` returns (so we are *not* inside `BeginMode3D`), draw screen-space overlays (ambient, lights, shadows).
- **UI pass**: `map_ptr->onDrawUI(dt)` runs last, so UI stays crisp/unaltered.

In pseudocode (structure only):

```cpp
GameLayer::onDraw(dt):
  clear_background(...)
  draw_world(dt)            // BeginMode3D/EndMode3D happens inside
  draw_lighting_overlay(dt) // 2D overlays on top of world
  draw_ui(dt)               // existing
```

##### How it “darkens” and “lights” pixels (blend modes)

We’ll rely on standard blend modes rather than rewriting materials:

- **Ambient darkening**: multiplicative blend (preferred) or alpha-black overlay (fallback).
  - Multiply is conceptually “brightness *= ambient”.
- **Local lights** (bar spill, windows): additive blend (adds light on top of the darkened world).
- **Shadows**: multiplicative darkening *only outdoors*.

In raylib terms this maps to blend modes roughly like:

- `BLEND_MULTIPLIED` for ambient/shadows
- `BLEND_ADDITIVE` (or `BLEND_ADD_COLORS`) for lights
- `BLEND_ALPHA` as a fallback if multiplied blending isn’t reliable on a target GPU

##### Key technical requirement: drawing shaped masks (not just rectangles)

To do “indoors vs outdoors” and to draw sun shadows, we need to draw **filled quads/polygons** in screen space:

- Convert world-space points to screen-space points using camera projection (raylib supports world→screen projection, e.g. `GetWorldToScreen*` equivalents).
- Render filled triangles via `rlgl` (already included in the project).

This is how we’ll draw:

- a roof rectangle mask (project 4 corners, draw 2 triangles),
- a sun shadow polygon (project points, triangulate),
- a bar light cone fan (project arc/fan points, draw triangle fan).

#### Option B (more flexible): Add a dedicated `lightRT` and composite in a shader

- **How**:
  - Render the world to `mainRT` (unchanged).
  - Render a grayscale/colored lightmask into a second render texture `lightRT`.
  - Composite: `final = world * ambientMask + additiveLight`, via a new shader or by extending post-processing.
- **Pros**:
  - Cleaner mental model: “world pass” + “lighting pass” + “post”.
  - Easy to blur lightmask cheaply (downsampled RT + simple blur).
  - Easier to tune and debug (can display `lightRT`).
- **Cons**:
  - Requires some plumbing: managing `lightRT`, binding as a second sampler, resizing on window changes.

**Recommendation**: start with **Option A** to get the look quickly, then move to **Option B** once the art direction is confirmed.

---

### Phase 1 — Global day/night ambience (no shadows yet)

#### Data needed

- **Time-of-day scalar**:
  - Use `HasDayNightTimer` to compute a value such as:
    - `is_day` = `timer.is_bar_closed()`
    - `t` = `timer.pct()` for a subtle dawn/dusk transition if desired.
- **Indoor/outdoor classification**:
  - Treat **any `Building` rectangle as “roofed/indoor”** for lighting purposes.
  - Outdoors = not inside any roofed building.

#### Visual design (starting values)

- **Night outdoors**:
  - Dark blue-gray ambient multiplier (e.g., 0.25–0.40 brightness).
- **Night indoors**:
  - Slightly brighter neutral/warm ambient (e.g., 0.55–0.75 brightness).
- **Day outdoors**:
  - Bright/warm ambient (e.g., 0.95–1.05 brightness).
- **Day indoors**:
  - Slightly reduced brightness vs outdoors (e.g., 0.80–0.95), no sun shadows.

#### Implementation sketch

**Option A implementation detail (practical and incremental)**:

1. **Compute lighting state from `HasDayNightTimer`**:
   - `is_day = timer.is_bar_closed()` (daytime)
   - `t = 1 - timer.pct()` (progress through the current phase; optional smoothing)
   - Derive:
     - `ambient_outdoor` (0..1)
     - `ambient_indoor` (0..1)
     - `night_tint` (optional; subtle blue at night)

2. **Apply a base ambient to the entire screen**:
   - Prefer: multiply whole screen by **outdoor** ambient at night (or ~1.0 at day).
   - This guarantees “outside looks like night” without needing per-object lighting.

3. **Make indoors different from outdoors** (without needing per-pixel classification):
   - We have building rectangles (`Building::area`) which represent “roofed zones”.
   - We project each building’s roof rectangle corners to screen and draw a filled quad mask.
   - Because multiply cannot brighten above the current value, the easiest stable layering is:
     - **Base pass**: multiply whole screen by `ambient_outdoor`.
     - **Indoor lift pass**: add a small indoor ambient “fill light” *inside roof rectangles only* using additive blending (or a lighter multiply if we choose base = indoor).

4. **Implementation for the roof mask quad**
   - Compute 4 world corners on the ground plane (XZ), using the building rectangle:
     - `(minX, y, minZ)`, `(maxX, y, minZ)`, `(maxX, y, maxZ)`, `(minX, y, maxZ)`
   - Use a consistent `y` (e.g., ground plane height used for your world).
   - Project each corner to screen with the active `Camera3D`.
   - Draw as 2 triangles (0-1-2 and 0-2-3) with `rlgl`.

---

### Phase 2 — Night “bar light spilling outside”

#### Light sources

- **Primary light**: warm cone/fan light just outside the bar entrance.
- **Secondary lights** (optional): window glows, small exterior fixtures.

#### Where to put the bar light

There are 2 robust ways; pick one:

- **A. Use building metadata**:
  - Add a door location for `BAR_BUILDING` (like `STORE_BUILDING.add_door(...)`).
  - Compute a world-space “door center” point from the rectangle edge.
- **B. Use world entities**:
  - Query for the actual `Door` entity/trigger area near the bar boundary and place the light at that transform.

**Recommendation**: start with **A** (simple) and later upgrade to **B** when door placement becomes data-driven.

#### Light rendering style

**Option A implementation detail**:

We render the bar light as an **additive cone/fan** in screen space, generated from world-space points near the bar entrance.

- **Choose an origin point** (world space):
  - For the initial version: derive a door point from `BAR_BUILDING` rectangle edge (add a door location like the store does).
  - Origin should sit slightly outside the bar boundary to sell “light spilling out”.

- **Generate cone geometry in world space**:
  - Define a direction vector pointing “out of the bar”.
  - Define:
    - `coneAngle` (e.g., 60–100 degrees)
    - `coneLength` (world units; how far it spills)
    - `numSegments` (e.g., 12–24)
  - Sample points along an arc at distance `coneLength` from origin.

- **Project to screen and draw**
  - Project origin + arc points to screen using the camera.
  - Draw a triangle fan: `(origin, p[i], p[i+1])`.

- **Make it soft**
  - Draw the cone multiple times from inside-out:
    - smaller cone, higher alpha
    - larger cone, lower alpha
  - This approximates a radial falloff without any blur pass.

- **Clamp to outdoors (avoid lighting “inside”)**
  - Simplest: do not draw any cone triangles whose centroid projects from a world point that is inside a roof rectangle (using `Building::is_inside` on the world-space sample points).

#### Occlusion (what blocks the light)

Phase 2 can initially ignore occlusion (light passes “through” objects) and still look good.
If we want quick occlusion later:

- Use a simple “walls as blockers” approximation:
  - Treat building rectangle edges as blockers for indoor/outdoor spill.
  - Clamp the cone to outside only (don’t draw inside the roofed area).

---

### Phase 3 — Day sunlight + outdoor-only shadows

#### Sun direction

- Fixed direction is fine (e.g., “sun from top-left”) to start.
- Optional: vary direction subtly based on day progression (`timer.pct()`).

#### Shadow types (start simple, then upgrade)

##### 3.1 Building roof shadows (cheap + high impact)

**Option A implementation detail (outdoor-only “roof shadow” illusion)**:

We treat each building rectangle as if it has an invisible roof that blocks sun, so the roof casts a shadow onto the ground outside.

1. **Define a sun direction on the ground plane**
   - Use a normalized 2D vector on XZ, e.g. `sunDirXZ = normalize(vec2(-1, -0.5))`.
   - Convert to 3D for sampling: `sunDir = vec3(sunDirXZ.x, 0, sunDirXZ.y)`.

2. **Compute a shadow polygon in world space**
   - Start from the 4 roof corners (same corners we use for the indoor mask).
   - Offset each corner by `(-sunDir * shadowLength)`.
   - Form a quad strip polygon connecting original edge(s) to offset edge(s).
   - Initial simplification (good enough visually):
     - Use a trapezoid built from the two corners on the “sun-facing” edge and their offsets.

3. **Project and draw as filled triangles**
   - Project the polygon vertices to screen.
   - Draw it as 2–4 triangles via `rlgl`.

4. **Apply only outdoors**
   - Since shadows are drawn as an overlay, we enforce “outdoors-only” by masking:
     - Draw shadows first (multiply darkening).
     - Then draw the indoor roof rectangles as a “shadow cancel” mask:
       - Either by redrawing indoors with no shadow effect (alpha blend),
       - Or by simply *not* drawing shadow triangles whose sample points are inside any roof rectangle.
   - Start with the simple rule: **skip shadow triangles whose world-space centroid is inside any roofed building**.

##### 3.2 Prop/entity shadows (optional)

- Fake contact shadows under entities as small soft ellipses.
- Still only apply outdoors (skip if entity position is inside any roofed building).

#### Softening

Once the hard-edged shadows look correct:

- Blur/soften without extra textures (Option A):
  - **Lights**: multiple additive cones/circles with decreasing alpha.
  - **Shadows**: draw 2–3 slightly offset/expanded shadow polygons with decreasing alpha to fake penumbra.

---

## Integration points (where this plugs into the codebase)

### Day/night state source

- `HasDayNightTimer` (owned by the named entity “Sophie”) is the authoritative day/night state.
- Existing triggers in `afterhours_day_night_transition_systems.cpp` can also be used to:
  - Toggle “night lighting enabled”
  - Precompute per-day lighting params

### Render pipeline hooks

- World is rendered in `GameLayer::draw_world(dt)` inside `BeginMode3D(...)`.
- `App` composes everything via `mainRT` then optional post-processing.

**Planned hook**:

- Add a “lighting overlay” step immediately after the world draw and before UI draw (still within the same render-to-texture frame), so UI remains unaffected by world lighting unless desired.

---

## Option A: concrete pass breakdown (what gets drawn, in what order)

This is the intended minimal pass sequence (written as a checklist your teammate can implement top-to-bottom):

1. **World pass**
   - Render the 3D world normally (whatever is currently in `draw_world(dt)` / map draw).

2. **Ambient base (night/day)**
   - Apply a fullscreen darkening/tint over the world.
   - **Night**: strong darken (prefer **multiply**, fallback to alpha-black tint if multiply doesn’t show reliably).
   - **Day**: either no-op or very subtle warm tint.

3. **Indoor lift (roofed regions)**
   - Treat each `Building::area` rectangle as a “roofed/indoor” region.
   - Project the 4 rectangle corners (world XZ) to screen and draw a filled quad (2 triangles).
   - Apply an **additive** “lift” color inside these quads so interiors are readable at night.

4. **Night local lights**
   - **Bar spill cone**:
     - Pick an origin near the bar entrance (door point pushed slightly outward).
     - Determine cone direction (door → outside).
     - Generate a triangle fan (arc points) and draw with **additive** blending.
     - Fake softness by drawing 2–3 layered cones with decreasing alpha / increasing radius.
   - Optional: window/fixture glows as small additive circles/fans.

5. **Day shadows (outdoor-only)**
   - Choose `sunDirXZ` (fixed direction is fine).
   - For each roofed building rectangle:
     - Build a shadow polygon by offsetting corners opposite sun direction.
     - Project to screen and draw filled triangles with **multiply** blending.
   - Enforce **outdoor-only**:
     - Skip any shadow triangles whose sampled/centroid world point lies inside any roofed rectangle, OR
     - Draw shadows first, then “cancel” indoors by overdrawing roof quads with a mask.

6. **UI pass**
   - Draw UI after lighting so UI is not darkened/tinted (unless explicitly desired).

7. **Debug tooling (highly recommended while building)**
   - Toggle to force-enable overlay in daytime.
   - Toggle to show projected roof quads + door/outside points + cone direction line.
   - Optional: show just the overlay (ambient/lights/shadows) for inspection.

---

## Controls & tuning (make it easy to iterate)

Add a small set of tweakables (initially as constants; later in settings/dev UI):

- **Night**:
  - Outdoor ambient multiplier
  - Indoor ambient multiplier
  - Bar light color, intensity, radius, cone angle, cone length
- **Day**:
  - Outdoor ambient multiplier
  - Indoor ambient multiplier
  - Sun direction (2D angle)
  - Shadow opacity + length

Also consider a debug view:

- Toggle to show the lightmask/shadow mask instead of the normal scene.

---

## Risks / unknowns to resolve early

- **Camera projection & screen-space drawing**:
  - If we draw masks in screen space, we need stable world→screen projection for corners/polygons.
- **Blend mode behavior**:
  - If Option A uses blend modes heavily, verify consistency across platforms.
- **Bar door placement**:
  - `BAR_BUILDING` currently doesn’t define door points; we’ll need a source of truth for where the light originates.
- **Aesthetic direction**:
  - Decide whether night is “moody realistic dark” vs “stylized readable dark”.

---

## Concrete milestone checklist

- **M1**: Night/day ambient shift (outside vs inside) with a smooth transition.
- **M2**: Bar door spill light visible outside at night.
- **M3**: Day sun direction established + building roof shadows outdoors only.
- **M4 (polish)**: Softened light/shadow edges + optional entity contact shadows.
- **M5 (tech debt)**: Move to `lightRT` composite shader if we started with direct overlay.

