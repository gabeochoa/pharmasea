
#pragma once
#include <string>

#include "entity.h"
#include "entity_helper.h"
#include "entity_makers.h"
#include "globals.h"

constexpr int MIN_MAP_SIZE = 10;
constexpr int MAX_MAP_SIZE = 25;
constexpr int MAX_SEED_LENGTH = 20;

extern std::vector<std::string> EXAMPLE_MAP;

struct LevelInfo {
    bool was_generated = false;

    game::State last_generated = game::State::InMenu;

    Entities entities;
    Entities::size_type num_entities;

    std::string seed;
    size_t hashed_seed;

    void update_seed(const std::string& s);
    void onUpdate(const Entities& players, float dt);
    void onDraw(float dt) const;
    void onDrawUI(float dt);
    void grab_things();
    void ensure_generated_map(const std::string& new_seed);

    // called by the server sometimes
    void generate_model_test_map();

   private:
    vec3 lobby_origin = {LOBBY_ORIGIN, 0, 0};
    vec3 progression_origin = {PROGRESSION_ORIGIN, 0, 0};
    vec3 model_test_origin = {MODEL_TEST_ORIGIN, 0, 0};
    vec3 store_origin = {STORE_ORIGIN, 0, 0};

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
        s.value8b(num_entities);
        s.container(entities, num_entities,
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);

        s.value8b(hashed_seed);
    }
};
