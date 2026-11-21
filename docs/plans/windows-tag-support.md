# Add Windows Tag Support to Remove Platform Checks

**Category:** System Consolidation
**Impact:** ~30 lines saved + platform consistency
**Risk:** Medium
**Complexity:** Medium

## Current State

6+ AI systems have platform-specific guards:

```cpp
#if !__APPLE__
if (entity.hasTag(AITransitionPending)) return;
if (entity.hasTag(AINeedsResetting)) return;
#endif
if (ctrl.state != ExpectedState) return;
```

The `#if !__APPLE__` suggests tags work on macOS but not Windows. This creates maintenance burden and inconsistent behavior across platforms.

## Refactoring

Add tag support to Windows build, then remove platform checks:

### Step 1: Investigate why tags don't work on Windows

Likely a build configuration or afterhours ECS issue. Check:
- MSVC template instantiation differences
- afterhours tag implementation
- Build flags

### Step 2: Fix Windows tag support

Whatever is blocking tags on Windows.

### Step 3: Remove all platform guards

Remove all `#if !__APPLE__` / `#if __APPLE__` guards around tag usage.

### Step 4: Extract utility function (now platform-agnostic)

```cpp
// In ai_utilities.h
inline bool ai_should_process(Entity& entity, const IsAIControlled& ctrl,
                              IsAIControlled::State expected_state) {
    if (entity.hasTag(AITransitionPending)) return false;
    if (entity.hasTag(AINeedsResetting)) return false;
    return ctrl.state == expected_state;
}

// Usage in systems:
void process(Entity& entity, float dt) {
    auto& ctrl = entity.get<IsAIControlled>();
    if (!ai_should_process(entity, ctrl, IsAIControlled::State::Wandering)) return;
    ai_tick_with_cooldown(entity, dt, 1.0f);
    // ... actual logic
}
```

## Impact

- Removes platform-specific code paths
- Consistent behavior across platforms
- Removes ~30 lines of duplicated guards across 6 systems
- Enables future tag usage without platform checks

## TODO

Investigate what's blocking tags on Windows:
- afterhours ECS config?
- MSVC template issue?
- Build flag missing?

## Files Affected

- Multiple AI system files in `src/system/ai/`
- Potentially afterhours ECS code
