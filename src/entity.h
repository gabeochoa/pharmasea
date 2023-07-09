
#pragma once

#include "engine/assert.h"
#include "external_include.h"
#include "strings.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <array>
#include <functional>
#include <map>

#include "components/base_component.h"
#include "engine/log.h"

using bitsery::ext::PointerObserver;
using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using StdMap = bitsery::ext::StdMap;

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;

    // These two functions can be used to validate than an entity has all of the
    // matching components that are needed for this system to run
    template<typename T>
    [[nodiscard]] bool has() const;

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
    [[nodiscard]] T& get() const;

    template<typename T>
    [[nodiscard]] T& get();

    virtual ~Entity() {}

    Entity() : id(ENTITY_ID_GEN++) {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s);
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

void register_all_components();
void add_entity_components(Entity* entity);
Entity* make_entity(const DebugOptions& options, vec3 p);
void add_person_components(Entity* person);
void add_player_components(Entity* player);
Entity* make_remote_player(vec3 pos);

void update_player_remotely(Entity& entity, float* location,
                            std::string username, int facing_direction);
void update_player_rare_remotely(Entity& entity, int model_index,
                                 int last_ping);
Entity* make_player(vec3 p);

Entity* make_aiperson(const DebugOptions& options, vec3 p);

Entity* make_customer(vec2 p, bool has_ailment);

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace entities {
Entity* make_furniture(const DebugOptions& options, vec2 pos, Color face,
                       Color base, bool is_);
Entity* make_table(vec2 pos);
Entity* make_character_switcher(vec2 pos);

Entity* make_wall(vec2 pos, Color c);

[[nodiscard]] Entity* make_conveyer(vec2 pos);

[[nodiscard]] Entity* make_grabber(vec2 pos);

[[nodiscard]] Entity* make_register(vec2 pos);

template<typename I>
[[nodiscard]] Entity* make_itemcontainer(const DebugOptions& options, vec2 pos);

[[nodiscard]] Entity* make_bagbox(vec2 pos);

[[nodiscard]] Entity* make_medicine_cabinet(vec2 pos);

[[nodiscard]] Entity* make_pill_dispenser(vec2 pos);

[[nodiscard]] Entity* make_trigger_area(vec3 pos, float width, float height,
                                        std::string title);

[[nodiscard]] Entity* make_customer_spawner(vec3 pos);

// This will be a catch all for anything that just needs to get updated
[[nodiscard]] Entity* make_sophie(vec3 pos);

}  // namespace entities
