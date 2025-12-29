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

#### Option A (fastest): Draw lighting directly onto `mainRT` after world render

- **How**:
  - After `GameLayer::draw_world(dt)` (still inside the render-to-texture pass), draw a fullscreen “darkness overlay”.
  - Use blend modes to “cut holes” (additive/alpha) for lights and to draw shadow shapes.
- **Pros**:
  - Minimal pipeline changes (no extra texture sampling in shaders required).
  - Easy to iterate visually.
- **Cons**:
  - Harder to do nice soft shadows/blur without an extra RT.
  - Some blending modes can be finicky across platforms/drivers.

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

- Add a small lighting module (e.g., `src/engine/lighting/…`) that exposes:
  - `LightingState { float outdoorAmbient; float indoorAmbient; vec3 sunDir; Color tint; }`
  - `Lighting::computeFromDayNightTimer(...)`
  - `Lighting::drawAmbientMask(...)`
- In the draw pipeline:
  - Draw world normally.
  - Apply ambient via either:
    - Option A: a fullscreen translucent overlay + “restore brightness” over indoor rectangles.
    - Option B: write ambient into `lightRT` as a base layer.

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

- Render lights into the lightmask as:
  - **Radial** falloff circles (cheap) for window/fixture glows.
  - **Cone** falloff (directional) for door spill:
    - Use a triangle/sector in world space projected to screen, with a soft gradient.
- Use a warm color temperature at night (yellow/orange), and keep intensity modest to avoid “overexposed bloom”.

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

- Treat each roofed building rectangle as an occluder casting a shadow onto the ground.
- Compute a projected shadow polygon on the XZ plane:
  - Take the rectangle corners in world space.
  - Offset them along \(-sunDir\) by a configurable “shadow length”.
  - Build a quad strip polygon (or a conservative trapezoid) and fill it.
- **Apply only outdoors**:
  - Do not draw any shadow pixels that lie inside roofed rectangles.
  - Easiest method: draw shadow, then “erase” it inside roofed regions (mask pass).

##### 3.2 Prop/entity shadows (optional)

- Fake contact shadows under entities as small soft ellipses.
- Still only apply outdoors (skip if entity position is inside any roofed building).

#### Softening

Once the hard-edged shadows look correct:

- Blur the shadow/light edges:
  - Option A: multi-pass draw of slightly expanded translucent polygons.
  - Option B: blur `lightRT` at half/quarter resolution, then composite.

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

- Add a “lighting overlay / lightRT generation” step immediately after the world draw and before UI draw, so UI remains unaffected by world lighting unless desired.

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

