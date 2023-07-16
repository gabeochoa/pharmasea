
#pragma once

#include "components/base_component.h"
//
#include "components/debug_name.h"
#include "components/transform.h"
//
// Item related components
#include "components/has_subtype.h"
#include "components/is_item.h"
//
//
#include "engine/assert.h"
#include "external_include.h"
#include "strings.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <array>
#include <map>

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using StdMap = bitsery::ext::StdMap;

#include "dataclass/names.h"
#include "drawing_util.h"
#include "engine.h"
#include "engine/astar.h"
#include "engine/is_server.h"
#include "engine/model_library.h"
#include "engine/util.h"
#include "globals.h"
#include "preload.h"
#include "raylib.h"
#include "text_util.h"
#include "vec_util.h"

using ComponentBitSet = std::bitset<max_num_components>;
// originally this was a std::array<BaseComponent*, max_num_components> but i
// cant seem to serialize this so lets try map
using ComponentArray = std::map<int, BaseComponent*>;
using Item = Entity;

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;

    ~Entity() {
        for (auto itr = componentArray.begin(); itr != componentArray.end();
             itr++) {
            if (itr->second) delete (itr->second);
        }
        componentArray.clear();
    }

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

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        log_trace("adding component_id:{} {} to entity_id: {}",
                  components::get_type_id<T>(), type_name<T>(), id);

        if (this->has<T>()) {
            log_warn(
                "This entity {}, {} already has this component attached id: "
                "{}, "
                "component {}",
                this->get<DebugName>(), id, components::get_type_id<T>(),
                type_name<T>());

            VALIDATE(false, "duplicate component");
            // Commented out on purpose because the assert is gonna kill the
            // program anyway at some point we should stop enforcing it to avoid
            // crashing the game when people are playing
            //
            // return this->get<T>();
        }

        T* component(new T(std::forward<TArgs>(args)...));
        componentArray[components::get_type_id<T>()] = component;
        componentSet[components::get_type_id<T>()] = true;

        log_trace("your set is now {}", componentSet);

        component->onAttach();

        return *component;
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

    template<typename T>
    [[nodiscard]] T& get() {
        if (this->is_missing<DebugName>()) {
            log_error(
                "This entity is missing debugname which will cause issues for "
                "if the get<> is missing");
        }
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                this->get<DebugName>(), id, components::get_type_id<T>(),
                type_name<T>());
        }
        BaseComponent* comp = componentArray.at(components::get_type_id<T>());
        return *static_cast<T*>(comp);
    }

    template<typename T>
    [[nodiscard]] const T& get() const {
        // TODO gotta be a way to merge these two
        if (this->is_missing<DebugName>()) {
            log_error(
                "This entity is missing debugname which will cause issues for "
                "if the get<> is missing");
        }
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                this->get<DebugName>(), id, components::get_type_id<T>(),
                type_name<T>());
        }
        BaseComponent* comp = componentArray.at(components::get_type_id<T>());
        return *static_cast<T*>(comp);
    }

    Entity() : id(ENTITY_ID_GEN++) {}

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
                  sv.ext(value, PointerOwner{PointerType::Nullable});
              });
    }
};

typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery

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

static bool check_name(const Entity& entity, const char* name) {
    return entity.get<DebugName>().name() == name;
}
