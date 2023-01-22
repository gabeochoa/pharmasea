
#pragma once
#include <array>
#include <map>

#include "bitsery/ext/std_smart_ptr.h"

//
#include "ailment.h"
#include "camera.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_have_ailment.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/has_name.h"
#include "components/is_snappable.h"
#include "components/model_renderer.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "drawing_util.h"
#include "engine/assert.h"
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/keymap.h"
#include "engine/log.h"
#include "engine/model_library.h"
#include "engine/sound_library.h"
#include "external_include.h"
#include "globals.h"
#include "item.h"
#include "item_helper.h"
#include "job.h"
#include "menu.h"
#include "names.h"
#include "preload.h"
#include "raylib.h"
#include "text_util.h"
#include "util.h"
#include "vec_util.h"

struct SpeechBubble {
    vec3 position;
    // TODO we arent using any string functionality can we swap to const char*?
    // perhaps in a typedef?
    std::string icon_tex_name;

    explicit SpeechBubble(std::string icon) : icon_tex_name(icon) {}

    void update(float, vec3 pos) { this->position = pos; }
    void render() const {
        GameCam cam = GLOBALS.get<GameCam>("game_cam");
        raylib::Texture texture = TextureLibrary::get().get(icon_tex_name);
        raylib::DrawBillboard(cam.camera, texture,
                              vec3{position.x,                      //
                                   position.y + (TILESIZE * 1.5f),  //
                                   position.z},                     //
                              TILESIZE,                             //
                              raylib::WHITE);
    }
};

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
    Entity(vec3 p) : id(ENTITY_ID_GEN++) {}
    Entity(vec2 p) : id(ENTITY_ID_GEN++) {}

    virtual ~Entity() {}

   protected:
    Entity() {}

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
        vec3 size = {TILESIZE, TILESIZE, TILESIZE};
        return get_bounds(this->get<Transform>().position, size);
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

   public:
    static Entity* create_entity(vec3 pos, Color face_color, Color base_color) {
        Entity* entity = new Entity(pos);
        entity->addComponent<Transform>();
        entity->addComponent<HasName>();
        entity->addComponent<CanHoldItem>();
        entity->addComponent<SimpleColoredBoxRenderer>();
        entity->addComponent<ModelRenderer>();
        entity->addComponent<CanBePushed>();

        entity->get<Transform>().init(pos, {TILESIZE, TILESIZE, TILESIZE});
        entity->get<SimpleColoredBoxRenderer>().init(face_color, base_color);
        return entity;
    }

    static Entity* create_person(vec3 pos, Color face_color, Color base_color) {
        // TODO move into component
        // s.value4b(model_index);
        std::array<std::string, 3> character_models = {
            "character_duck",
            "character_dog",
            "character_bear",
        };
        int model_index = 0;
        // void select_next_character_model() {
        // model_index = (model_index + 1) % character_models.size();
        // }

        Entity* person = new Entity(pos);
        person->addComponent<Transform>();
        person->addComponent<HasName>();
        person->addComponent<CanHoldItem>();
        person->addComponent<ModelRenderer>();
        person->addComponent<CanBePushed>();

        person->addComponent<SimpleColoredBoxRenderer>().init(face_color,
                                                              base_color);
        person->addComponent<HasBaseSpeed>().update(10.f);

        const float sz = TILESIZE * 0.75f;
        person->get<Transform>().init(pos, {sz, sz, sz});

        // log_info("model index: {}", model_index);
        // TODO add a component for this
        person->get<ModelRenderer>().update(ModelInfo{
            // TODO fix this
            .model_name = character_models[model_index],
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });

        return person;
    }

    static Entity* create_aiperson(vec3 pos, Color face_color,
                                   Color base_color) {
        Entity* aiperson = new Entity(pos);

        aiperson->addComponent<Transform>();
        aiperson->addComponent<HasName>();
        aiperson->addComponent<CanHoldItem>();
        aiperson->addComponent<ModelRenderer>();
        aiperson->addComponent<CanBePushed>();
        aiperson->addComponent<SimpleColoredBoxRenderer>().init(face_color,
                                                                base_color);
        aiperson->addComponent<CanPerformJob>().update(Wandering, Wandering);
        aiperson->addComponent<HasBaseSpeed>().update(10.f);

        return aiperson;
    }

    static Entity* create_customer(vec2 pos, Color base_color) {
        Entity* customer = new Entity({pos.x, 0, pos.y});

        customer->addComponent<Transform>();
        customer->addComponent<HasName>();
        customer->addComponent<CanHoldItem>();
        customer->addComponent<ModelRenderer>();
        customer->addComponent<CanBePushed>();

        customer->addComponent<SimpleColoredBoxRenderer>().init(base_color,
                                                                base_color);
        customer->addComponent<CanPerformJob>().update(Wandering, Wandering);
        customer->addComponent<HasBaseSpeed>().update(10.f);

        customer->addComponent<CanHaveAilment>().update(
            std::make_shared<Insomnia>());

        customer->get<HasName>().update(get_random_name());
        customer->get<CanPerformJob>().update(WaitInQueue, Wandering);

        // TODO turn back on bubbles
        // std::optional<SpeechBubble> bubble;
        // bubble = SpeechBubble(ailment->icon_name());

        // TODO support this
        // virtual void in_round_update(float dt) override {
        // AIPerson::in_round_update(dt);

        // Register* reg = get_target_register();
        // if (reg) {
        // this->turn_to_face_entity(reg);
        // }
        //
        // if (bubble.has_value())
        // bubble.value().update(dt, this->get<Transform>().raw_position);
        // }

        // virtual void render_normal() const override {
        // auto render_speech_bubble = [&]() {
        // if (!this->bubble.has_value()) return;
        // this->bubble.value().render();
        // };
        // AIPerson::render_normal();
        // render_speech_bubble();
        // }

        return customer;
    }
};

typedef Entity Person;
typedef Person AIPerson;
typedef AIPerson Customer;
typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery
