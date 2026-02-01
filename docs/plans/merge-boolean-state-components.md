# Merge Boolean State Components

**Category:** Component Consolidation
**Impact:** ~200 lines saved
**Risk:** Very Low
**Complexity:** Low

## Current State

4 components follow identical pattern:
- `CanBeGhostPlayer` - `bool ghost`
- `CanBeHeld` - `bool held`
- `CanBeHighlighted` - `bool highlighted`
- `CanBeTakenFrom` - `bool allowed`

Each has ~50 lines with `is_*()`, `is_not_*()`, `update(bool)` methods.

## Refactoring

Create a template:

```cpp
template<typename Tag>
struct BoolState : public BaseComponent {
    bool value = false;

    bool is_set() const { return value; }
    bool is_not_set() const { return !value; }
    void update(bool v) { value = v; }

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(static_cast<BaseComponent&>(self), self.value);
    }
};

// Type aliases maintain semantic clarity
using CanBeGhostPlayer = BoolState<struct GhostPlayerTag>;
using CanBeHeld = BoolState<struct HeldTag>;
using CanBeHighlighted = BoolState<struct HighlightedTag>;
using CanBeTakenFrom = BoolState<struct TakeableTag>;
```

## Impact

Reduces ~200 LOC to ~30 LOC + 4 type aliases

## Why This Is Low Risk

Existing code works unchanged due to type aliases. The API remains the same, only the implementation is shared.

## Files Affected

- `src/components/can_be_ghost_player.h`
- `src/components/can_be_held.h`
- `src/components/can_be_highlighted.h`
- `src/components/can_be_taken_from.h`
