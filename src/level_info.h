
#pragma once
#include <string>

#include "entity.h"
#include "entity_helper.h"
#include "entity_makers.h"

constexpr int MIN_MAP_SIZE = 10;
constexpr int MAX_MAP_SIZE = 25;
constexpr int MAX_SEED_LENGTH = 20;

extern std::vector<std::string> EXAMPLE_MAP;

struct LevelInfo {
    bool was_generated = false;

    game::State last_generated = game::State::InMenu;

    std::string seed;
    size_t hashed_seed;

    void update_seed(const std::string& s);
    void onUpdate(const Entities& players, float dt);
    void onDraw(float dt) const;
    void onDrawUI(float dt);
    void ensure_generated_map(const std::string& new_seed);

    // called by the server sometimes
    void generate_model_test_map();
    // called by the server sometimes
    void generate_load_save_room_map();

   private:
    void generate_lobby_map();
    void generate_progression_map();
    void generate_store_map();
    void generate_default_seed();

    void generate_in_game_map();
    auto get_rand_walkable();
    auto get_rand_walkable_register();
    void add_outside_triggers(vec2 origin);

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // NOTE: we no longer store/duplicate an entity list in LevelInfo.
        // The authoritative entity store lives in the active EntityCollection
        // (EntityHelper::get_current_collection()).
        //
        // For compatibility with existing save/network formats, we still
        // serialize a full entity list as part of LevelInfo, but:
        // - on write: read entities from the active collection
        // - on read: deserialize into a temporary vector and then install it
        //   into the active collection via replace_all_entities()
        Entities::size_type num_entities = 0;
        if constexpr (requires { s.adapter().error(); }) {
            // Deserializer path: read entities and install into active world.
            Entities tmp;
            s.value8b(num_entities);
            s.container(tmp, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });

            EntityHelper::get_current_collection().replace_all_entities(
                std::move(tmp));
        } else {
            // Serializer path: write entities from active world.
            auto& ents = EntityHelper::get_entities_for_mod();
            num_entities = ents.size();
            s.value8b(num_entities);
            s.container(ents, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });
        }
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);

        s.value8b(hashed_seed);
    }
};
