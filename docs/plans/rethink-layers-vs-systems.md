# Rethink Layers - Keep, Refactor, or Convert to Systems?

**Category:** Layer Architecture
**Impact:** ~500+ lines saved (potentially)
**Risk:** High (full conversion) / Low (base classes only)
**Complexity:** High

## Current State

20 layers exist as a separate abstraction from ECS systems. With the event system gone, what value do layers still provide?

## What Layers Currently Do

Layers handle:
- **UI Rendering** - menus, HUD, overlays, debug displays
- **Screen Management** - which screen is active (menu, game, pause, settings)
- **Input Routing** - different input handling per screen
- **State Transitions** - menu navigation, pause/unpause

## The Question: Why Not Just Use Systems?

Systems already handle update/render loops. If layers are just "systems that check game state before running," maybe they should be systems.

**What layers provide that systems don't:**
1. **Explicit render order** - layers draw in stack order (back to front)
2. **Modal behavior** - pause layer can block input to game layer
3. **Separation of concerns** - UI code vs game logic code
4. **Easy enable/disable** - layer visibility is explicit

**What systems provide that layers don't:**
1. **ECS integration** - can query entities, use components
2. **Consistent execution model** - same should_run/process pattern
3. **No separate abstraction** - one less concept to understand

## Approach Options

### A. Keep Layers, Add Base Classes (Original Proposal)

```cpp
struct MenuLayer : public UILayer {
    menu::State required_state;
    // ... base class handles boilerplate
};
```

- Least disruptive
- Still have two abstractions (layers + systems)

### B. Convert Layers to UI Systems

Each layer becomes a system that renders UI:

```cpp
struct AboutScreenSystem : public System<> {
    std::shared_ptr<ui::UIContext> ui_context;
    SVGRenderer svg;

    bool should_run(float) override {
        return MenuState::get().is(menu::State::About);
    }

    void run(float dt) override {
        handle_back_navigation();
        ext::clear_background(ui::UI_THEME.background);
        ui::begin(ui_context, dt);
        svg.render(ui_context);
        ui::end();
    }
};
```

- Unifies the model
- UI systems run after game systems (render order via system registration order)
- Modal behavior via system execution order and input consumption

### C. Hybrid - Keep Layer for Render Stack, Systems for Logic

```cpp
// Minimal layer - just defines render order
struct UIRenderLayer : public Layer {
    void onDraw(float dt) override {
        // Systems have already updated state
        // Layer just handles draw order
        SystemManager::get().run_ui_render_systems(dt);
    }
};

// Logic lives in systems
struct AboutScreenInputSystem : public System<> { /* handle input */ };
struct AboutScreenRenderSystem : public System<> { /* render */ };
```

- Clear separation: systems for logic, layer for ordering
- More complex but explicit

### D. Remove Layers Entirely

- All UI becomes ECS entities with render components
- `UIWidget` entities instead of procedural UI code
- Menu buttons are entities with `CanBeClicked` components
- Radical but fully ECS-native

## Questions to Answer

1. **Is render order the main value of layers?** Could we achieve this with system priorities instead?

2. **Is the modal/blocking behavior important?** How do we prevent game input while pause menu is up?

3. **Should UI be entities?** Or is procedural UI (immediate mode) better for menus?

4. **What about layers that mix logic and rendering?** (e.g., SettingsLayer manages state + renders UI)

5. **Migration cost** - 20 layers to convert. Is it worth it?

## Current Layer Inventory

| Layer | Purpose | Complexity | Convert to System? |
|-------|---------|------------|-------------------|
| MenuLayer | Main menu | Low | Easy |
| AboutLayer | About screen | Low | Easy |
| SettingsLayer | Settings UI + state | High | Medium |
| NetworkLayer | Lobby/network UI | High | Medium |
| GameLayer | Main game loop | High | Already system-like |
| PauseLayer | Pause overlay | Low | Easy |
| RecipeBookLayer | Recipe UI overlay | Low | Easy |
| ToastLayer | Toast notifications | Low | Easy |
| FPSLayer | FPS counter | Low | Easy |
| GameDebugLayer | Debug info | Low | Easy |
| DebugSettingsLayer | Debug UI | Medium | Medium |
| HandLayer | Cursor rendering | Low | Easy |
| VersionLayer | Version display | Low | Easy |
| StreamerSafeLayer | Streamer mode | Low | Easy |
| MapViewerLayer | Map viewer | Medium | Medium |
| SeedManagerLayer | Seed management | Medium | Medium |
| RoundEndReasonLayer | Round end UI | Low | Easy |
| UITestLayer | UI testing | Low | Easy |

## Recommendation

**Start with Option B (Convert to Systems)** for simple layers first:
1. Convert `FPSLayer`, `VersionLayer`, `HandLayer`, `ToastLayer` to systems
2. Evaluate: is the system approach cleaner?
3. If yes, continue converting
4. If no, fall back to Option A (base classes)

The key insight: if layers are just "systems with state guards," make them systems. If they provide unique value (render ordering, modal behavior), keep them but simplify.

## Files Affected

- `src/layers/` (20 files)
- `src/engine/app.h` (layer registration)
