#pragma once

#include "../entity.h"
#include "../entity_ref.h"

namespace system_manager::ai {

template<typename T>
inline T& ensure_component(Entity& e) {
    if (e.is_missing<T>()) {
        e.addComponent<T>();
    }
    return e.get<T>();
}

template<typename T>
inline void reset_component(Entity& e) {
    e.removeComponentIfExists<T>();
    e.addComponent<T>();
}

[[nodiscard]] inline bool entity_ref_valid(const EntityRef& ref) {
    if (ref.empty()) return false;
    return static_cast<bool>(ref.resolve());
}

}  // namespace system_manager::ai

