
#pragma once

#include "bitsery/ext/utils/pointer_utils.h"
#include "engine/assert.h"
#include "external_include.h"
#include "strings.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <array>
#include <functional>
#include <map>

#include "engine/log.h"
#include "engine/type_name.h"

struct BaseComponent;

constexpr int max_num_components = 64;
using ComponentBitSet = std::bitset<max_num_components>;
// originally this was a std::array<BaseComponent*, max_num_components> but
// i cant seem to serialize this so lets try map
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

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using StdMap = bitsery::ext::StdMap;

static std::atomic_int ENTITY_ID_GEN = 0;

struct DebugName;

struct Entity {
    int id;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;

    Entity() : id(ENTITY_ID_GEN++) {}

    // Destructor
    // ~Entity();
    // Copy constructor
    // Entity(const Entity& other) = delete;
    // Entity& operator=(const Entity&) = delete;

    // move constructor
    // Entity(Entity&& other)
    // : id(std::move(other.id)),
    // componentSet(std::move(other.componentSet)),
    // componentArray(std::move(other.componentArray)),
    // cleanup(std::move(other.cleanup)) {}
    //
    // Entity& operator=(Entity&& other) {
    // if (this != &other) {
    // id = std::move(other.id);
    // componentSet = std::move(other.componentSet);
    // componentArray = std::move(other.componentArray);
    // cleanup = std::move(other.cleanup);
    // }
    // return *this;
    // }

    // Copy assignment operator
    // Entity& operator=(const Entity& other);

    // These two functions can be used to validate than an entity has all of
    // the matching components that are needed for this system to run

    template<typename T>
    [[nodiscard]] bool has() const {
        log_trace("checking component {} {} on entity {}",
                  ::components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[::components::get_type_id<T>()];
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

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args);

    template<typename A>
    void addAll() {
        addComponent<A>();
    }

    template<typename A, typename B, typename... Rest>
    void addAll() {
        addComponent<A>();
        addAll<B, Rest...>();
    }

    template<typename T>
    [[nodiscard]] T& get() {
        if (this->is_missing<DebugName>()) {
            log_error(
                "This entity is missing debugname which will cause issues "
                "for "
                "if the get<> is missing");
        }
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                name(), id, components::get_type_id<T>(), type_name<T>());
        }
        BaseComponent* comp = componentArray.at(components::get_type_id<T>());
        return *static_cast<T*>(comp);
    }

    // TODO combine with non const
    template<typename T>
    [[nodiscard]] const T& get() const {
        if (this->is_missing<DebugName>()) {
            log_error(
                "This entity is missing debugname which will cause issues "
                "for "
                "if the get<> is missing");
        }
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                name(), id, components::get_type_id<T>(), type_name<T>());
        }
        BaseComponent* comp = componentArray.at(components::get_type_id<T>());
        return *static_cast<T*>(comp);
    }

    const std::string& name() const;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.ext(componentSet, bitsery::ext::StdBitset{});
        s.value1b(cleanup);

        s.ext(componentArray, StdMap{max_num_components},
              [](S& sv, int& key, BaseComponent*(&value)) {
                  sv.value4b(key);
                  // sv.ext(value, PointerOwner{PointerType::Nullable});
                  sv.ext(value, PointerObserver{PointerType::Nullable});
              });
    }
};

struct DebugOptions {
    std::string name;
};

typedef std::reference_wrapper<Entity> RefEntity;
typedef std::optional<std::reference_wrapper<Entity>> OptEntity;

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

inline bool valid(OptEntity opte) { return opte.has_value(); }

inline Entity& asE(OptEntity opte) { return opte.value(); }
inline OptEntity asOpt(Entity& e) { return std::make_optional(std::ref(e)); }

inline Entity& asE(RefEntity& refe) { return refe.get(); }
inline RefEntity asRef(Entity& e) { return std::ref(e); }

bool check_name(const Entity&, const char*);

void update_player_remotely(Entity& entity, float* location,
                            std::string username, int facing_direction);
void update_player_rare_remotely(Entity& entity, int model_index,
                                 int last_ping);

void register_all_components();
void add_entity_components(Entity& entity);
void add_person_components(Entity& person);
void add_player_components(Entity& player);

Entity& make_entity(Entity& entity, const DebugOptions& options, vec3 p);
Entity& make_remote_player(Entity& entity, vec3 pos);
Entity& make_player(Entity& entity, vec3 p);

Entity& make_aiperson(Entity& entity, const DebugOptions& options, vec3 p);

Entity& make_customer(Entity& entity, vec2 p, bool has_ailment);

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace entities {
Entity& make_furniture(Entity& entity, const DebugOptions& options, vec2 pos,
                       Color face, Color base, bool is_);
Entity& make_table(Entity& entity, vec2 pos);
Entity& make_character_switcher(Entity& entity, vec2 pos);

Entity& make_wall(Entity& entity, vec2 pos, Color c);

Entity& make_conveyer(Entity& entity, vec2 pos);

Entity& make_grabber(Entity& entity, vec2 pos);

Entity& make_register(Entity& entity, vec2 pos);

template<typename I>
Entity& make_itemcontainer(Entity& entity, const DebugOptions& options,
                           vec2 pos);

Entity& make_bagbox(Entity& entity, vec2 pos);

Entity& make_medicine_cabinet(Entity& entity, vec2 pos);

Entity& make_pill_dispenser(Entity& entity, vec2 pos);

Entity& make_trigger_area(Entity& entity, vec3 pos, float width, float height,
                          std::string title);

Entity& make_customer_spawner(Entity& entity, vec3 pos);

// This will be a catch all for anything that just needs to get updated
Entity& make_sophie(Entity& entity, vec3 pos);

}  // namespace entities
