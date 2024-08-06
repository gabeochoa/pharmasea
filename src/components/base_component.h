
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
constexpr int max_num_components = 128;
using ComponentID = int;

namespace components {
namespace internal {
inline ComponentID get_unique_id() noexcept {
    static ComponentID lastID{0};
    // TODO this doesnt work for some reason
    // if (lastID + 1 > max_num_components)
    // log_error(
    // "You are trying to add a new component but you have used up all "
    // "the space allocated, updated max_num");
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
