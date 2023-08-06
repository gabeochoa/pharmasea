
#pragma once

#include "entity.h"
#include "entity_makers.h"
#include "entityhelper.h"
#include "system/system_manager.h"

constexpr int MIN_MAP_SIZE = 5;
constexpr int MAX_MAP_SIZE = 20;
constexpr int MAX_SEED_LENGTH = 20;

extern std::vector<std::string> EXAMPLE_MAP;

static void generate_and_insert_walls(std::string /* seed */) {
    // TODO generate walls based on seed
    const auto d_color = (Color){155, 75, 0, 255};
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
            if (i == 0 || j == 0 || i == MAX_MAP_SIZE - 1 ||
                j == MAX_MAP_SIZE - 1) {
                vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                furniture::make_wall(EntityHelper::createEntity(), location,
                                     d_color);
            }
        }
    }
}

struct LevelInfo {
    bool was_generated = false;

    game::State last_generated = game::State::InMenu;

    Entities entities;
    Entities::size_type num_entities;

    std::string seed;

    //
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    void update_seed(const std::string& s);
    void onUpdate(Entities& players, float dt);
    void onDraw(float dt) const;
    void onDrawUI(float dt);
    void grab_things();
    void ensure_generated_map(const std::string& new_seed);

   private:
    vec3 lobby_origin = {LOBBY_ORIGIN, 0, LOBBY_ORIGIN};

    void generate_lobby_map();
    void generate_default_seed();

    [[nodiscard]] int gen_rand(int a, int b) {
        return a + (generator() % (b - a));
    }

    void generate_in_game_map();
    auto get_rand_walkable();
    auto get_rand_walkable_register();

   private:
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
        // TODO these arent serializable...
        // s.object(generator);
        // s.object(dist);
    }
};
