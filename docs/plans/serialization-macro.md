# Serialization Macro for Simple Components

**Category:** Serialization
**Impact:** ~300 lines saved
**Risk:** Very Low
**Complexity:** Very Low

## Current State

65+ components have identical serialization boilerplate:

```cpp
friend zpp::bits::access;
constexpr static auto serialize(auto& archive, auto& self) {
    return archive(                         //
        static_cast<BaseComponent&>(self),  //
        self.field1,                        //
        self.field2                         //
    );
}
```

## Refactoring

Macro for common case:

```cpp
#define COMPONENT_SERIALIZE(...) \
    friend zpp::bits::access; \
    constexpr static auto serialize(auto& archive, auto& self) { \
        return archive(static_cast<BaseComponent&>(self), __VA_ARGS__); \
    }

// Usage:
struct HasPatience : public BaseComponent {
    float max_patience = 100.f;
    float current_patience = 100.f;

    COMPONENT_SERIALIZE(self.max_patience, self.current_patience)
};
```

For components with no fields:

```cpp
#define COMPONENT_SERIALIZE_EMPTY() \
    friend zpp::bits::access; \
    constexpr static auto serialize(auto& archive, auto& self) { \
        return archive(static_cast<BaseComponent&>(self)); \
    }
```

## Impact

Reduces 3-7 lines per component across 65+ components (~300 lines total)

## Why This Is Very Low Risk

- Pure syntactic sugar
- Expands to exactly the same code
- No behavior change
- Easy to grep for if ever need to undo

## Files Affected

- All files in `src/components/` (65+ files)
- New macro definition location TBD (possibly `src/components/component_macros.h`)
