
#pragma once

#include "bitsery/ext/std_smart_ptr.h"
#include "components/base_component.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_hold_item.h"
#include "components/has_name.h"
#include "components/model_renderer.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "engine/assert.h"
#include "external_include.h"
//
#include <array>
#include <map>

#include "drawing_util.h"
#include "engine/astar.h"
#include "engine/is_server.h"
#include "engine/model_library.h"
#include "globals.h"
#include "item.h"
#include "item_helper.h"
#include "menu.h"
#include "preload.h"
#include "raylib.h"
#include "text_util.h"
#include "util.h"
#include "vec_util.h"

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;
    bool is_held = false;

    template<typename T>
    bool has() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        log_info("adding component {} {} to entity {}",
                 components::get_type_id<T>(), type_name<T>(), id);

        M_ASSERT(!this->has<T>(),
                 "This entity already has this component attached");

        std::shared_ptr<T> component;
        component.reset(new T(std::forward<TArgs>(args)...));
        // TODO figure out why this causes double free
        // component->entity = std::shared_ptr<Entity>(this);
        componentArray[components::get_type_id<T>()] = component;
        componentSet[components::get_type_id<T>()] = true;

        log_trace("your set is now {}", componentSet);

        return *component;
    }

    template<typename T>
    T& get() const {
        if (!has<T>()) {
            log_error("Entity {} did not have component {} requested", id,
                      type_name<T>());
        }
        auto ptr(componentArray[components::get_type_id<T>()]);
        return *dynamic_pointer_cast<T>(ptr);
    }

    void add_static_components() {
        addComponent<Transform>();
        addComponent<HasName>();
        addComponent<CanHoldItem>();
        addComponent<SimpleColoredBoxRenderer>();
        addComponent<ModelRenderer>();
        addComponent<CanBePushed>();
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.container(componentArray,
                    [](S& sv, std::shared_ptr<BaseComponent> bc) {
                        sv.ext(bc, bitsery::ext::StdSmartPtr{});
                    });
        s.ext(componentSet, bitsery::ext::StdBitset{});
        s.value1b(cleanup);
        s.value1b(is_held);
    }

   public:
    Entity(vec3 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++) {
        add_static_components();
        get<Transform>().init(p, size());
        get<SimpleColoredBoxRenderer>().init(face_color_in, base_color_in);
    }

    Entity(vec2 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++) {
        add_static_components();
        get<Transform>().init({p.x, 0, p.y}, size());
        get<SimpleColoredBoxRenderer>().init(face_color_in, base_color_in);
    }

    Entity(vec3 p, Color c) : id(ENTITY_ID_GEN++) {
        add_static_components();
        get<Transform>().init(p, size());
        get<SimpleColoredBoxRenderer>().init(c, c);
    }

    Entity(vec2 p, Color c) : id(ENTITY_ID_GEN++) {
        add_static_components();
        get<Transform>().init({p.x, 0, p.y}, size());
        get<SimpleColoredBoxRenderer>().init(c, c);
    }

    virtual ~Entity() {}

   protected:
    Entity() {
        add_static_components();
        get<Transform>().init({0, 0, 0}, size());
        get<SimpleColoredBoxRenderer>().init(BLACK, BLACK);
    }

    virtual vec3 size() const { return (vec3){TILESIZE, TILESIZE, TILESIZE}; }

    virtual vec2 get_heading() {
        const float target_facing_ang =
            util::deg2rad(this->get<Transform>().FrontFaceDirectionMap.at(
                this->get<Transform>().face_direction));
        return vec2{
            cosf(target_facing_ang),
            sinf(target_facing_ang),
        };
    }

    virtual void turn_to_face_entity(Entity* target) {
        if (!target) return;

        // dot product visualizer https://www.falstad.com/dotproduct/

        // the angle between two vecs is
        // @ = arccos(  (a dot b) / ( |a| * |b| ) )

        // first get the headings and normalise so |x| = 1
        const vec2 my_heading = vec::norm(this->get_heading());
        const vec2 tar_heading = vec::norm(target->get_heading());
        // dp = ( (a dot b )/ 1)
        float dot_product = vec::dot2(my_heading, tar_heading);
        // arccos(dp)
        float theta_rad = acosf(dot_product);
        float theta_deg = util::rad2deg(theta_rad);
        int turn_degrees = (180 - (int) theta_deg) % 360;
        // TODO fix this
        (void) turn_degrees;
        if (turn_degrees > 0 && turn_degrees <= 45) {
            this->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(0);
        } else if (turn_degrees > 45 && turn_degrees <= 135) {
            this->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(90);
        } else if (turn_degrees > 135 && turn_degrees <= 225) {
            this->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(180);
        } else if (turn_degrees > 225) {
            this->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(270);
        }
    }

    virtual void in_round_update(float) {}

    // Whether or not this entity can hold items at the moment
    // -- doesnt need to be static, can dynamically change values
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) {
        return false;
    }

   public:
    virtual void announce(std::string text) {
        // TODO have some way of distinguishing between server logs and regular
        // client logs
        if (is_server()) {
            log_info("server: {}: {}", this->id, text);
        } else {
            // log_info("client: {}: {}", this->id, text);
        }
    }

    /*
     * Given another bounding box, check if it collides with this entity
     *
     * @param BoundingBox the box to test
     * */
    virtual bool collides(BoundingBox b) const {
        // TODO fix move to collision component
        return CheckCollisionBoxes(this->get<Transform>().bounds(), b);
    }

    [[nodiscard]] virtual BoundingBox raw_bounds() const {
        // TODO fix  size
        return this->get<Transform>().raw_bounds();
    }

    /*
     * Get the bounding box for this entity
     * @returns BoundingBox the box
     * */
    [[nodiscard]] virtual BoundingBox bounds() const {
        // TODO fix  size
        return get_bounds(this->get<Transform>().position, this->size());
    }

    [[nodiscard]] vec3 snap_position() const {
        // TODO have callers go direct
        return this->get<Transform>().snap_position();
    }

    /*
     * Rotate the facing direction of this entity, clockwise 90 degrees
     * */
    void rotate_facing_clockwise() {
        this->get<Transform>().face_direction =
            this->get<Transform>().offsetFaceDirection(
                this->get<Transform>().face_direction, 90);
    }

    /*
     * Returns the location of the tile `distance` distance in front of the
     * entity
     *
     * @param int, how far in front to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    virtual vec2 tile_infront(int distance) {
        // TODO fix snap
        vec2 tile = vec::to2(this->snap_position());
        return tile_infront_given_pos(tile, distance,
                                      this->get<Transform>().face_direction);
    }

    /*
     * Given a tile, distance, and direction, returns the location of the
     * tile `distance` distance in front of the tile
     *
     * @param vec2, the starting location
     * @param int, how far in front to go
     * @param Transform::FrontFaceDirection, which direction to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    static vec2 tile_infront_given_pos(
        vec2 tile, int distance,
        Transform::Transform::FrontFaceDirection direction) {
        if (direction & Transform::FORWARD) {
            tile.y += distance * TILESIZE;
            tile.y = ceil(tile.y);
        }
        if (direction & Transform::BACK) {
            tile.y -= distance * TILESIZE;
            tile.y = floor(tile.y);
        }

        if (direction & Transform::RIGHT) {
            tile.x += distance * TILESIZE;
            tile.x = ceil(tile.x);
        }
        if (direction & Transform::LEFT) {
            tile.x -= distance * TILESIZE;
            tile.x = floor(tile.x);
        }
        return tile;
    }

    // TODO not currently used as we disabled navmesh
    virtual bool add_to_navmesh() { return false; }
    // return true if the item has collision and is currently collidable
    virtual bool is_collidable() {
        // by default we disable collisions when you are holding something
        // since its generally inside your bounding box
        return !is_held;
    }
    // Used to tell an entity its been picked up
    virtual void on_pickup() { this->is_held = true; }

    // Used to tell and entity its been dropped and where to go next
    virtual void on_drop(vec3 location) {
        this->is_held = false;
        this->get<Transform>().update(vec::snap(location));
    }

    virtual void update(float dt) final {
        TRACY_ZONE_SCOPED;
        if (GameState::get().is(game::State::InRound)) {
            in_round_update(dt);
        }
    }
};

typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery
