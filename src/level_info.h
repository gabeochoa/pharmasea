
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
        // Compatibility note: the save/network format still includes a full
        // entity list. We no longer store that list on LevelInfo; we serialize
        // from/to the active EntityCollection (EntityHelper).

        // Bitsery "read vs write" split:
        // - input adapters expose `error()` (ReaderError)
        // - output adapters do not
        //
        // Use a named concept so this isn't "magic" inside the function body.
        constexpr bool kIsReader = requires { s.adapter().error(); };

        Entities::size_type num_entities = 0;
        if constexpr (kIsReader) {
            Entities tmp;
            s.value8b(num_entities);
            s.container(tmp, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });

            EntityHelper::get_current_collection().replace_all_entities(
                std::move(tmp));
        } else {
            // Write path: serialize a stable copy of the current entity list.
            // (This preserves the old "grab_things copied a vector" behavior,
            // without storing the copy on LevelInfo.)
            Entities tmp = EntityHelper::get_entities();
            num_entities = tmp.size();
            s.value8b(num_entities);
            s.container(tmp, num_entities,
                        [](S& s2, std::shared_ptr<Entity>& entity) {
                            s2.ext(entity, bitsery::ext::StdSmartPtr{});
                        });
        }
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);

        s.value8b(hashed_seed);
    }
};
