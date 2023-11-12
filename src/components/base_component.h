
#pragma once

#include <array>
#include <bitset>
#include <map>
#include <memory>

#include "../bitsery_include.h"
#include "../globals.h"

struct Entity;
using Entities = std::vector<std::shared_ptr<Entity>>;
using bitsery::ext::PointerObserver;

struct BaseComponent;
constexpr int max_num_components = 64;
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
    Entity* parent = nullptr;

    BaseComponent() {}
    BaseComponent(BaseComponent&&) = default;

    void attach_parent(Entity* p) {
        parent = p;
        onAttach();
    }

    virtual void onAttach() {}
    virtual ~BaseComponent() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(parent, PointerObserver{});
    }
};
