
#pragma once

#include "bitsery/ext/std_smart_ptr.h"
#include "components/base_component.h"
#include "engine/assert.h"
#include "external_include.h"
//
#include <array>
#include <map>

#include "components/transform.h"
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

    vec3 pushed_force{0.0, 0.0, 0.0};
    Color face_color;
    Color base_color;
    bool cleanup = false;
    bool is_highlighted = false;
    bool is_held = false;
    std::shared_ptr<Item> held_item = nullptr;

    //
    int name_length = 1;
    std::string name = "";

    template<typename T>
    bool hasComponent() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        M_ASSERT(!this->hasComponent<T>(),
                 "This entity already has this component attached");

        log_trace("adding component {} {} to entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);

        std::shared_ptr<T> component =
            std::make_shared<T>(std::forward<TArgs>(args)...);
        component->entity = std::shared_ptr<Entity>(this);
        componentArray[components::get_type_id<T>()] = component;
        componentSet[components::get_type_id<T>()] = true;

        log_trace("your set is now {}", componentSet);

        return *component;
    }

    template<typename T>
    T& get() const {
        if (!hasComponent<T>()) {
            log_error("Entity {} did not have component {} requested", id,
                      type_name<T>());
        }
        auto ptr(componentArray[components::get_type_id<T>()]);
        return *dynamic_pointer_cast<T>(ptr);
    }

    void add_static_components() { addComponent<Transform>(); }

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
        s.object(pushed_force);
        s.object(face_color);
        s.object(base_color);
        s.value1b(cleanup);
        s.value1b(is_highlighted);
        s.value1b(is_held);
        s.object(held_item);
        s.value4b(name_length);
        s.text1b(name, name_length);
    }

   public:
    Entity(vec3 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++),
          face_color(face_color_in),
          base_color(base_color_in) {
        add_static_components();
        get<Transform>().init(p, size());
    }

    Entity(vec2 p, Color face_color_in, Color base_color_in)
        : id(ENTITY_ID_GEN++),
          face_color(face_color_in),
          base_color(base_color_in) {
        add_static_components();
        get<Transform>().init({p.x, 0, p.y}, size());
    }

    Entity(vec3 p, Color c)
        : id(ENTITY_ID_GEN++), face_color(c), base_color(c) {
        add_static_components();
        get<Transform>().init(p, size());
    }

    Entity(vec2 p, Color c)
        : id(ENTITY_ID_GEN++), face_color(c), base_color(c) {
        add_static_components();
        get<Transform>().init({p.x, 0, p.y}, size());
    }

    virtual ~Entity() {}

    void update_name(const std::string& new_name) {
        name = new_name;
        name_length = (int) name.size();
    }

   protected:
    Entity() {
        add_static_components();
        get<Transform>().init({0, 0, 0}, size());
    }

    virtual vec3 size() const { return (vec3){TILESIZE, TILESIZE, TILESIZE}; }

    /*
     * Used for code that should only render when debug mode is on
     * */
    virtual void render_debug_mode() const {
        DrawBoundingBox(this->bounds(), MAROON);
        DrawFloatingText(this->get<Transform>().raw_position,
                         Preload::get().font,
                         fmt::format("{}", this->id).c_str());
    }

    /*
     * Used for code for when the entity is highlighted
     * */
    virtual void render_highlighted() const {
        TRACY_ZONE_SCOPED;
        if (model().has_value()) {
            ModelInfo model_info = model().value();

            Color base = ui::color::getHighlighted(this->base_color);

            float rotation_angle =
                // TODO make this api better
                180.f + static_cast<int>(
                            this->get<Transform>().FrontFaceDirectionMap.at(
                                this->get<Transform>().face_direction));

            DrawModelEx(model_info.model,
                        {
                            this->get<Transform>().position.x +
                                model_info.position_offset.x,
                            this->get<Transform>().position.y +
                                model_info.position_offset.y,
                            this->get<Transform>().position.z +
                                model_info.position_offset.z,
                        },
                        vec3{0.f, 1.f, 0.f}, rotation_angle,
                        this->size() * model_info.size_scale, base);
            return;
        }

        Color f = ui::color::getHighlighted(this->face_color);
        Color b = ui::color::getHighlighted(this->base_color);
        DrawCubeCustom(this->get<Transform>().raw_position, this->size().x,
                       this->size().y, this->size().z,
                       this->get<Transform>().FrontFaceDirectionMap.at(
                           this->get<Transform>().face_direction),
                       f, b);
    }

    /*
     * Used for normal gameplay rendering
     * */
    virtual void render_normal() const {
        TRACY_ZONE_SCOPED;
        if (this->is_highlighted) {
            render_highlighted();
            return;
        }

        if (model().has_value()) {
            ModelInfo model_info = model().value();

            float rotation_angle =
                // TODO make this api better
                180.f + static_cast<int>(
                            this->get<Transform>().FrontFaceDirectionMap.at(
                                this->get<Transform>().face_direction));

            raylib::DrawModelEx(
                model_info.model,
                {
                    this->get<Transform>().position.x +
                        model_info.position_offset.x,
                    this->get<Transform>().position.y +
                        model_info.position_offset.y,
                    this->get<Transform>().position.z +
                        model_info.position_offset.z,
                },
                vec3{0, 1, 0}, model_info.rotation_angle + rotation_angle,
                this->size() * model_info.size_scale, this->base_color);
        } else {
            DrawCubeCustom(this->get<Transform>().raw_position, this->size().x,
                           this->size().y, this->size().z,
                           this->get<Transform>().FrontFaceDirectionMap.at(
                               this->get<Transform>().face_direction),
                           this->face_color, this->base_color);
        }

        render_floating_name();
    }

    virtual void render_floating_name() const {
        raylib::DrawFloatingText(
            this->get<Transform>().raw_position + vec3{0, 0.5f * TILESIZE, 0},
            Preload::get().font, name.c_str());
    }

    virtual std::optional<ModelInfo> model() const { return {}; }

    virtual void update_held_item_position() {
        if (held_item != nullptr) {
            auto new_pos = this->get<Transform>().position;
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            if (this->get<Transform>().face_direction &
                Transform::FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_item->update_position(new_pos);
        }
    }

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

    virtual void always_update(float) {
        is_highlighted = false;
        if (this->is_snappable()) {
            this->get<Transform>().position = this->snap_position();
        } else {
            this->get<Transform>().position =
                this->get<Transform>().raw_position;
        }
        update_held_item_position();
    }

    virtual void planning_update(float) {}
    virtual void in_round_update(float) {}

    // return true if the position should be snapped at every update
    virtual bool is_snappable() { return false; }
    // Whether or not this entity can hold items at the moment
    // -- doesnt need to be static, can dynamically change values
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) {
        return false;
    }

   public:
    // Whether or not this entity has something we can take from them
    virtual bool can_take_item_from() const {
        return this->held_item != nullptr;
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

    /*
     * Move the entity to the given position,
     *
     * @param vec3 the position to move to
     * */
    virtual void update_position(const vec3& p) {
        // TODO fix
        this->get<Transform>().raw_position = p;
    }

    [[nodiscard]] virtual BoundingBox raw_bounds() const {
        // TODO fix  size
        return get_bounds(this->get<Transform>().raw_position, this->size());
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
        this->update_position(vec::snap(location));
    }

    virtual void render() const final {
        TRACY_ZONE_SCOPED;
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);

        if (!debug_mode_on) {
            render_normal();
        }

        if (debug_mode_on) {
            render_debug_mode();
        }
    }

    virtual void update(float dt) final {
        TRACY_ZONE_SCOPED;
        // TODO do we run game updates during paused?
        // TODO rename game/nongame to in_round inplanning
        if (GameState::get().is(game::State::InRound)) {
            in_round_update(dt);
        } else {
            planning_update(dt);
        }
        always_update(dt);
    }
};

typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery
