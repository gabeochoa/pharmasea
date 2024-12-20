
#pragma once

#include "engine.h"
#include "external_include.h"
//
#include "entity_type.h"
//
#include "components/base_component.h"
//
#include "engine/assert.h"
#include "engine/log.h"
#include "engine/type_name.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <map>

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using StdMap = bitsery::ext::StdMap;

using ComponentBitSet = std::bitset<max_num_components>;
// originally this was a std::array<BaseComponent*, max_num_components> but i
// cant seem to serialize this so lets try map
using ComponentArray = std::map<int, std::unique_ptr<BaseComponent>>;
using Item = Entity;
using EntityID = int;

// TODO memory? we could keep track of deleted entities and reuse those ids if
// we wanted to we'd have to be pretty disiplined about clearing people who
// store ids...

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    // TODO :INFRA: go around and audit id uses
    EntityID id;

    EntityType type = EntityType::Unknown;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;

    Entity() : id(ENTITY_ID_GEN++) {}
    ~Entity();
    Entity(const Entity&) = delete;
    Entity(Entity&& other) noexcept = default;

    // These two functions can be used to validate than an entity has all of the
    // matching components that are needed for this system to run
    template<typename T>
    [[nodiscard]] bool has() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename A, typename B, typename... Rest>
    bool has() const {
        return has<A>() && has<B, Rest...>();
    }

    template<typename T>
    [[nodiscard]] bool is_missing() const {
        return !has<T>();
    }

    template<typename A>
    bool is_missing_any() const {
        return is_missing<A>();
    }

    template<typename A, typename B, typename... Rest>
    bool is_missing_any() const {
        return is_missing<A>() || is_missing_any<B, Rest...>();
    }

    template<typename T>
    void removeComponent() {
        log_info("removing component_id:{} {} to entity_id: {}",
                 components::get_type_id<T>(), type_name<T>(), id);
        if (!this->has<T>()) {
            log_error(
                "trying to remove but this entity {} {} doesnt have the "
                "component attached {} {}",
                name(), id, components::get_type_id<T>(), type_name<T>());
        }
        componentSet[components::get_type_id<T>()] = false;
        // BaseComponent* ptr = componentArray[components::get_type_id<T>()];
        componentArray.erase(components::get_type_id<T>());
        // if (ptr) delete ptr;
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        log_trace("adding component_id:{} {} to entity_id: {}",
                  components::get_type_id<T>(), type_name<T>(), id);

        // checks for duplicates
        if (this->has<T>()) {
            log_warn(
                "This entity {}, {} already has this component attached id: "
                "{}, "
                "component {}",
                name(), id, components::get_type_id<T>(), type_name<T>());

            VALIDATE(false, "duplicate component");
            // Commented out on purpose because the assert is gonna kill the
            // program anyway at some point we should stop enforcing it to avoid
            // crashing the game when people are playing
            //
            // return this->get<T>();
        }

        // non uinque ptr
        // T* component(new T(std::forward<TArgs>(args)...));
        // componentArray[components::get_type_id<T>()] = component;
        // componentSet[components::get_type_id<T>()] = true;

        auto component = std::make_unique<T>(std::forward<TArgs>(args)...);
        componentArray[components::get_type_id<T>()] = std::move(component);
        componentSet[components::get_type_id<T>()] = true;

        log_trace("your set is now {}", componentSet);

        componentArray[components::get_type_id<T>()]->attach_parent(this);

        return get<T>();
    }

    template<typename T, typename... TArgs>
    T& addComponentIfMissing(TArgs&&... args) {
        if (this->has<T>()) return this->get<T>();
        return addComponent<T>(std::forward<TArgs>(args)...);
    }

    template<typename T>
    void removeComponentIfExists() {
        if (this->is_missing<T>()) return;
        return removeComponent<T>();
    }

    template<typename A>
    void addAll() {
        addComponent<A>();
    }

    template<typename A, typename B, typename... Rest>
    void addAll() {
        addComponent<A>();
        addAll<B, Rest...>();
    }

    const std::string_view name() const;

    template<typename T>
    void warnIfMissingComponent() const {
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                name(), id, components::get_type_id<T>(), type_name<T>());
        }
    }

    template<typename T>
    [[nodiscard]] T& get() {
        warnIfMissingComponent<T>();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-stack-address"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-stack-address"
        return static_cast<T&>(
            *componentArray.at(components::get_type_id<T>()).get());
    }

    template<typename T>
    [[nodiscard]] const T& get() const {
        warnIfMissingComponent<T>();

        return static_cast<const T&>(
            *componentArray.at(components::get_type_id<T>()).get());
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.value4b(type);

        s.ext(componentSet, bitsery::ext::StdBitset{});
        s.value1b(cleanup);

        s.ext(componentArray, StdMap{max_num_components},
              [](S& sv, int& key, std::unique_ptr<BaseComponent>(&value)) {
                  sv.value4b(key);
                  // sv.ext(value, PointerOwner{PointerType::Nullable});
                  sv.ext(value, bitsery::ext::StdSmartPtr{});
              });
    }
};

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery

struct DebugOptions {
    EntityType type = EntityType::Unknown;
    bool enableCharacterModel = true;
};

using RefEntity = std::reference_wrapper<Entity>;
using OptEntityType = std::optional<std::reference_wrapper<Entity>>;

struct OptEntity {
    OptEntityType data;

    OptEntity() {}
    OptEntity(OptEntityType opt_e) : data(opt_e) {}
    OptEntity(RefEntity _e) : data(_e) {}
    OptEntity(Entity& _e) : data(_e) {}

    bool has_value() const { return data.has_value(); }
    bool valid() const { return has_value(); }

    Entity* value() { return &(data.value().get()); }
    Entity* operator*() { return value(); }
    Entity* operator->() { return value(); }

    const Entity* value() const { return &(data.value().get()); }
    const Entity* operator*() const { return value(); }
    // TODO look into switching this to using functional monadic stuff
    // i think optional.transform can take and handle calling functions on an
    // optional without having to expose the underlying value or existance of
    // value
    const Entity* operator->() const { return value(); }

    Entity& asE() { return data.value(); }
    const Entity& asE() const { return data.value(); }

    operator RefEntity() { return data.value(); }
    operator RefEntity() const { return data.value(); }
    operator bool() const { return valid(); }
};

namespace bitsery {
template<typename S>
void serialize(S& s, RefEntity ref) {
    Entity& e = ref.get();
    Entity* eptr = &e;
    s.ext8b(eptr, PointerObserver{});
}

template<typename S>
void serialize(S& s, OptEntity opt) {
    s.ext(opt, bitsery::ext::StdOptional{});
}
}  // namespace bitsery

bool check_type(const Entity& entity, EntityType type);
bool check_if_drink(const Entity& entity);
