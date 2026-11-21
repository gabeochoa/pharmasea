# Afterhours onboarding report — bump 4c86b3a → 4d8da9e

Date: 2026-11-17. No pharmasea source code was changed for this report.
The only working-tree change is the pulled submodule itself.

## What was pulled

| | Value |
|---|---|
| Last committed bump (superproject `HEAD`) | `4c86b3a1` (2026-10-25, e2e serial dispatch) |
| Pre-pull working-tree checkout | `cb000ec` (2026-10-01) — older than the recorded bump, an ancestor of it; pulling forward discards that stale detach, nothing was lost |
| Pulled to | `origin/main` `4d8da9e` (2026-11-17, Merge PR #90 controlled-widgets) |
| Delta size | 325 commits (183 non-merge), 369 files, +16,405 / −760 |
| Library source delta | 72 files under `vendor/afterhours/src/` (50 plugins, 10 backends, 3 core); 9 new headers, 0 deleted |
| Rest of delta | 198 example files, 90 test files — reference material, not code to import |

Upstream's own summary of this delta is the `## Unreleased` section of
`vendor/afterhours/CHANGELOG.md`. This report filters it against what
pharmasea actually uses: ECS core (`ah.h`, `Library<T>`), input
(`plugins/input_system.h`, layered mapping in `src/input_mapping_persistence.h`),
window/gestures, sound libraries (`src/preload.cpp`), files/atomic writes,
logging shim (`src/log/log.h`), and the e2e input injector
(`src/engine/simulated_input/`). Pharmasea does **not** use the imm UI
plugin — its UI is hand-rolled — which makes most of the delta inapplicable.

## Blocker found (not fixed, per report-only scope)

The game does not build against the new pin. Observed with `make -j8`:

```
vendor/afterhours/src/core/system.h:684:9: error: use of undeclared identifier 'VALIDATE'
vendor/afterhours/src/plugins/input_system.h:1127:13: error: use of undeclared identifier 'VALIDATE'
```

Cause: `src/pch.hpp` defines `AFTER_HOURS_REPLACE_VALIDATE`, so the vendor
fallback in `vendor/afterhours/src/logging.h:105` is suppressed, but
`src/log/log.h` never provided a `VALIDATE`. New library headers now use
`VALIDATE` on pharmasea's include path (`SystemManager::fixed_tick_rate`,
`input::Repeat`). Pharmasea's own `VALIDATE` exists in
`src/engine/assert.h:8` but is included too late. Recommended fix (one
include, not applied here): pull `engine/assert.h` in before the first
vendor header, or drop `AFTER_HOURS_REPLACE_VALIDATE` and take the vendor
fallback. Nothing else in the delta is a compile blocker for pharmasea:
all six upstream breaking changes (animation `motion` rewrite,
`HasScrollView::viewport_size` optional, label `text_inset`, `Margin`
field order, deleted `colors::to_hsl/to_rgb/get_highlighted`) are UI-only
or already migrated (`get_highlighted` → `lighten` in the last bump).

## Free wins — already in, no code to write

- `should_run` const-override forwarding (`core/system.h`) — const-only
  systems are gated again; pharmasea was bitten by this class before.
- Chord specificity — Ctrl+Shift+Z no longer also fires Ctrl+Z
  (`input_system.h`); benefits the settings remap screen.
- `Library::load` duplicate-name leak fixed and `whereID` O(1) seeding,
  `has_child_of`/`get_with_child` 18× fewer dynamic casts, backend
  `measure_text` memo (99.8% hit rate upstream) — pharmasea deletes
  entities and runs 25+ custom queries per frame, so these land directly.
- `files::write_string_atomic` durability (unique temp, fsync, rename) —
  already adopted in `src/preload.cpp:228` and `src/engine/settings.cpp:253`.
  Note: a save now waits on disk, tens of ms on macOS.
- Font loading: headless segfault guard, glyph fallback across loaded
  faces, visible U+FFFD replacement, atlas mipmaps/trilinear filtering,
  expanded `default_codepoints()` (Latin-1, arrows, checks). Benefits the
  Korean font and any symbol glyphs in pharmasea's own text drawing.
- `load_cjk_fonts` 4-byte UTF-8 fix — emoji/CJK Ext-B glyphs in
  translations now load.

## Onboard — worth adopting, in priority order

| # | Feature (upstream) | Pharmasea evidence | Effort | Why |
|---|---|---|---|---|
| 1 | `VALIDATE` shim fix (blocker above) | build error `system.h:684`, `input_system.h:1127` | S | Nothing else matters until this compiles |
| 2 | `plugins/frustum.h`: `make_frustum(camera, aspect)` / `make_frustum(view_projection)`, `contains_point` / `intersects_sphere` / `intersects_box` | Own copy `src/engine/frustum.h` (raylibExtras, `fetch_data()` + point/sphere/AABB); live `static Frustum frustum` in `src/system/rendering/rendering_system.cpp:1497` | S–M | Same math, upstream-tested against raylib matrices on 60k points; deletes a vendored copy |
| 3 | `plugins/models.h`: `InstanceBatcher` + `model_transform` + `draw_model_instanced` | Per-entity `DrawModelEx` at `rendering_system.cpp:311` and `:1105`; many repeated furniture/machine models | M | Upstream: ~8k instanced vs ~1.5k per-draw copies at 60 fps. Keep pharmasea's own `ModelLibrary` (`src/libraries/model_library.h` — glTF loader, fallback cube, lazy loading); upstream explicitly says own libraries can stay. Batcher sits beside it, grouped by model address |
| 4 | `plugins/particles.h`: `particles::Emitter<N>` fixed pool (gravity/drag/floor/restitution, exact drag) | Open tracker item "Particle system (effects for drinks, vomit, etc.)" in `docs/active/todo.md` core features | M | Directly closes an existing todo with a tested pool instead of a new hand-rolled system |
| 5 | `plugins/lod.h`: `Lod::update` / `update_sq` with hysteresis margin | No LOD today; pairs with #2 culling in the render loop | S | Stops threshold flicker; `update_sq` fits loops that already have squared distance |
| 6 | `src/random_engine.h` (upstream): `RngStream` with `save()`/`load()` state string, `EntityQuery::gen_random(stream)` | Own single global `RandomEngine` (pcg32 + mt19937, `src/engine/random_engine.h:39`) shared by map generation (`src/map_generation.cpp:709`) and AI (`src/system/ai/`) | M | Per-domain streams (map gen vs AI vs store) isolate determinism; saveable state helps save files and the replay-validation harness. Caveat: upstream sampling uses stdlib distributions, which can differ across standard libraries — keep pcg for anything network/replay-critical |
| 7 | E2E harness: `parse_e2e_args` `--key=value`/`--e2e-timeout`, `register_command` typed args, `E2ERunner::results()` + `write_results_json` + `--results-json`, `testing::is_e2e_running()`, `wait_until`/`wait_wall`, `hold_key`/`release_key`, screenshot-failure fails script, serial dispatch | Hand parser `src/engine/simulated_input/e2e_manager.h` handles only `wait`/`wait_frames`/`key`, warns "unhandled command" for the rest; already on upstream `input_injector` | M–L | Adopt incrementally: results JSON + `is_e2e_running()` first (machine-readable CI signal), then typed commands. Full UI-command adoption still blocked on imm UI, same conclusion as `docs/plans/afterhours-main-upgrade.md` |
| 8 | `files::TestRoots` (RAII save/config/temp redirect) + `files::get_cache_path()` + `settings::set_path_override` | Own sago-based `Files` (`src/engine/files.cpp:49`); unit tests in `src/tests/` run against the real save folder today | S | Tests stop polluting/leaking into the real `pharmasea` save root; cache path separates regenerable data from settings |
| 9 | Input: `input::is_held/is_pressed(Action)` enum form, `input::Repeat{delay, rate}`, `set_binding`/`clear_binding`/`actions_using` + binding equality | Layered mapping `ProvidesLayeredInputMapping<menu::State>` (`src/input_mapping_persistence.h:53`); settings screen hand-rolls key/button capture (`src/layers/settingslayer.cpp:364`) | S–M | Conflict detection before rebinding in settings; `Repeat` replaces hand timers for held menu/gamepad repeat. Layered map already had remapping — this is the conflict-check half |
| 10 | `SystemManager::fixed_tick_rate` / `max_fixed_ticks` | Fixed step was a hard 1/120 constant with uncapped catch-up (a 2 s hitch = 240 fixed updates in one frame) | S | Set a catch-up cap once the tick path is confirmed; prevents spiral-of-death hitches. Verify current tick wiring first — no `fixed_tick` references exist in `src/` today |
| 11 | `src/image.h`: `image_size` / `decode_image` / `load_texture_from_rgba` | Texture loading in `Preload::load_textures` (`src/preload.cpp:714`) | S | Header-only dimension reads (sprite aspect without GPU decode) and one-call pixel upload; adopt opportunistically, not as a migration |
| 12 | `FrameRateStats` + font extras (`load_font_from_memory`, `font_coverage::font_size_per_point`, `load_font_with_codepoints` size, `set_default_font`) | Own `FontLibrary`; deterministic e2e screenshots want a stable FPS readout | S | Small, self-contained; `font_size_per_point` ends hand-calibrated point sizes |

## Skip — evaluated, not worth it for pharmasea

| Feature | Reason |
|---|---|
| Whole imm UI suite: `color_swatch` trio, `TreeView::render_row`, `ellipsize_text`, grapheme editing, `grab_initial_focus`, `FocusProxy`, `is_middle_click`, `virtual_list` overscan, menu marks/scrolling, pseudo-locale, `os::appearance`, `effects`/blur, `with_shader`, UI `motion` animation rewrite | Pharmasea does not use the imm UI plugin; adopting any one piece means adopting the UI stack. Revisit only with a UI migration decision (already tracked under "afterhours gaps") |
| `house_of_cards` impostor cards | Real perf headroom (65k copies/19 ms upstream) but only after #3 instancing; bake pipeline + not pixel-identical close up. Re-evaluate if instancing is not enough |
| Lit/fogged models (`load_lit_shader`) | Pharmasea has its own lighting runtime and shaders (`src/lighting_runtime.cpp`, lighting.vs/fs in preload) |
| Sound wrappers (`StopSound`/`SetSoundPitch`/… , `synth::beep`) | No current call sites need stop/pitch/pan; adopt the day PA/music ducking does |
| Toast plugin fade fix | Pharmasea has its own `src/engine/toastmanager.h`; upstream fix only affects the unused plugin |
| `whereIndexed`/`add_index` query indexes | Attractive for rope/queue child lookups, but indexes rebuild on collection change and need `invalidate_indexes()` discipline; only with a measured hot scan |
| `capture::enable` / `expect_drawn*` / `expect_point_in` | Assertion layer is built on imm UI commands; pharmasea e2e stays injector + screenshots (#7) |
| `colors::` raylib palette names, `Vector4Type`/`MatrixType`/`Camera2D` macros | Pharmasea already wraps raylib types itself (`src/engine/graphics.h`); no sokol port planned |

## Suggested sequence

1. Fix the `VALIDATE` blocker, build, run `./pharmasea.exe -t` and one
   `.e2e` script — the bump is then landed.
2. Frustum swap (#2) and `max_fixed_ticks` check (#10) — small, isolated.
3. `TestRoots` for tests (#8) before the next test-writing session.
4. `InstanceBatcher` pilot on the most-repeated model (#3), then `Lod` (#5).
5. Particles for drink/vomit effects (#4) — closes an existing todo.
6. E2E results JSON (#7) and `RngStream` for map generation (#6) as
   standalone projects.
