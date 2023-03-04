
#pragma once

#include <array>
#include <bitset>
#include <memory>

#include "../vendor_include.h"

struct Entity;
struct BaseComponent;

constexpr int max_num_components = 64;
using ComponentBitSet = std::bitset<max_num_components>;
using ComponentArray =
    std::array<std::shared_ptr<BaseComponent>, max_num_components>;
using ComponentID = int;

namespace components {
namespace internal {
inline ComponentID get_unique_id() noexcept {
    static ComponentID lastID{0};
    return lastID++;
}

}  // namespace internal

template<typename T>
inline ComponentID get_type_id() noexcept {
    static_assert(std::is_base_of<BaseComponent, T>::value,
                  "T must inherit from BaseComponent");
    static ComponentID typeID{internal::get_unique_id()};
    return typeID;
}
}  // namespace components

struct BaseComponent {
    // std::shared_ptr<Entity> entity;

    BaseComponent() {}
    // BaseComponent(const BaseComponent& other) : entity(other.entity) {}
    BaseComponent(BaseComponent&&) = default;

    virtual void onAttach() {}

    virtual ~BaseComponent() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S&) {
        // s.object(entity);
    }
};
