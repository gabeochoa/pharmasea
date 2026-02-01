# Consolidate AI Timer/Cooldown Components

**Category:** Component Consolidation
**Impact:** ~60 lines saved
**Risk:** Low
**Complexity:** Low

## Current State

3 components are structurally identical wrappers around `CooldownInfo`:
- `HasAICooldown` - just `CooldownInfo cooldown{}`
- `HasAIDrinkState` - just `CooldownInfo timer{}`
- `HasAIWanderState` - just `CooldownInfo timer{}`

Each is ~30 lines doing the same thing with different field names.

## Refactoring

Single generic timer component with purpose enum:

```cpp
enum class AITimerPurpose : uint8_t {
    General,
    Drinking,
    Wandering
};

struct HasAITimer : public BaseComponent {
    CooldownInfo timer{};
    AITimerPurpose purpose = AITimerPurpose::General;

    // Delegate to CooldownInfo methods
    bool is_ready() const { return timer.is_ready(); }
    void reset() { timer.reset(); }
    void update(float dt) { timer.update(dt); }

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self), self.timer, self.purpose);
    }
};

// Or simpler - just use CooldownInfo directly as a component if allowed
```

Alternatively, if these need to coexist on the same entity, keep them separate but use a template:

```cpp
template<typename Tag>
struct AITimerComponent : public BaseComponent {
    CooldownInfo timer{};
    // ... shared implementation
};

using HasAICooldown = AITimerComponent<struct CooldownTag>;
using HasAIDrinkState = AITimerComponent<struct DrinkStateTag>;
using HasAIWanderState = AITimerComponent<struct WanderStateTag>;
```

## Impact

Reduces ~90 lines to ~30 lines

## Files Affected

- `src/components/has_ai_cooldown.h`
- `src/components/has_ai_drink_state.h`
- `src/components/has_ai_wander_state.h`
