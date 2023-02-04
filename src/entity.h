
#pragma once

#include "bitsery/ext/std_smart_ptr.h"
#include "components/base_component.h"
#include "components/can_be_held.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_be_taken_from.h"
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

    template<typename T>
    [[nodiscard]] bool has() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename T>
    [[nodiscard]] bool is_missing() const {
        return !has<T>();
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        log_trace("adding component {} {} to entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);

        // TODO eventually enforce this
        if (this->has<T>()) {
            log_warn("This entity already has this component attached");
            return this->get<T>();
        }
        // M_ASSERT(!this->has<T>(),
        // "This entity already has this component attached");

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
    [[nodiscard]] T& get() const {
        if (!has<T>()) {
            log_error("Entity {} did not have component {} requested", id,
                      type_name<T>());
        }
        auto ptr(componentArray[components::get_type_id<T>()]);
        return *dynamic_pointer_cast<T>(ptr);
    }

    void add_static_components(vec3 pos) {
        addComponent<Transform>().init(pos, size());
        addComponent<HasName>();
        addComponent<CanHoldItem>();
        addComponent<SimpleColoredBoxRenderer>();
        addComponent<ModelRenderer>();
        addComponent<CanBePushed>();
        addComponent<CanBeHeld>();
        addComponent<CanBeTakenFrom>();
    }

    virtual ~Entity() {}

    Entity(vec3 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++) {
        add_static_components(p);
        get<SimpleColoredBoxRenderer>().init(face_color_in, base_color_in);
    }

    Entity(vec3 p, Color c) : Entity(p, c, c) {}

    Entity() {
        add_static_components({0, 0, 0});
        get<SimpleColoredBoxRenderer>().init(BLACK, BLACK);
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
    }

   protected:
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

    static void turn_to_face_entity(std::shared_ptr<Entity> entity,
                                    Entity* target) {
        if (!target) return;

        // dot product visualizer https://www.falstad.com/dotproduct/

        // the angle between two vecs is
        // @ = arccos(  (a dot b) / ( |a| * |b| ) )

        // first get the headings and normalise so |x| = 1
        const vec2 my_heading = vec::norm(entity->get_heading());
        const vec2 tar_heading = vec::norm(target->get_heading());
        // dp = ( (a dot b )/ 1)
        float dot_product = vec::dot2(my_heading, tar_heading);
        // arccos(dp)
        float theta_rad = acosf(dot_product);
        float theta_deg = util::rad2deg(theta_rad);
        int turn_degrees = (180 - (int) theta_deg) % 360;
        // TODO fix entity
        (void) turn_degrees;
        if (turn_degrees > 0 && turn_degrees <= 45) {
            entity->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(0);
        } else if (turn_degrees > 45 && turn_degrees <= 135) {
            entity->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(90);
        } else if (turn_degrees > 135 && turn_degrees <= 225) {
            entity->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(180);
        } else if (turn_degrees > 225) {
            entity->get<Transform>().face_direction =
                static_cast<Transform::FrontFaceDirection>(270);
        }
    }

    // Whether or not this entity can hold items at the moment
    // -- doesnt need to be static, can dynamically change values
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) {
        return false;
    }

   public:
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

    /*
     * Rotate the facing direction of this entity, clockwise 90 degrees
     * */
    static void rotate_facing_clockwise(std::shared_ptr<Entity> entity) {
        entity->get<Transform>().face_direction =
            entity->get<Transform>().offsetFaceDirection(
                entity->get<Transform>().face_direction, 90);
    }

    /*
     * Returns the location of the tile `distance` distance in front of the
     * entity
     *
     * @param entity, the entity to get the spot in front of
     * @param int, how far in front to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    static vec2 tile_infront_given_player(std::shared_ptr<Entity> entity,
                                          int distance) {
        vec2 tile = vec::to2(entity->get<Transform>().snap_position());
        return tile_infront_given_pos(tile, distance,
                                      entity->get<Transform>().face_direction);
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
};

typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery

// TODO from customer
// std::optional<SpeechBubble> bubble;
// struct SpeechBubble {
// vec3 position;
// // TODO we arent using any string functionality can we swap to const
// char*?
// // perhaps in a typedef?
// std::string icon_tex_name;
//
// explicit SpeechBubble(std::string icon) : icon_tex_name(icon) {}
//
// void update(float, vec3 pos) { this->position = pos; }
// void render() const {
// GameCam cam = GLOBALS.get<GameCam>("game_cam");
// raylib::Texture texture = TextureLibrary::get().get(icon_tex_name);
// raylib::DrawBillboard(cam.camera, texture,
// vec3{position.x,                      //
// position.y + (TILESIZE * 1.5f),  //
// position.z},                     //
// TILESIZE,                             //
// raylib::WHITE);
// }
// };
//
