
#pragma once

#include <array>
#include <bitset>
#include <map>
#include <memory>

#include "../entity.h"
#include "../globals.h"
#include "../vendor_include.h"

struct BaseComponent;

constexpr int max_num_components = 64;
using ComponentBitSet = std::bitset<max_num_components>;
// originally this was a std::array<BaseComponent*, max_num_components> but i
// cant seem to serialize this so lets try map
using ComponentArray = std::map<int, BaseComponent*>;
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
    BaseComponent() {}
    BaseComponent(BaseComponent&&) = default;

    virtual void onAttach() {}

    virtual ~BaseComponent() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S&) {}
};
