
#pragma once

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

#include "engine/globals_register.h"
#include "engine/ui_color.h"
#include "entity.h"
#include "entityhelper.h"
#include "furniture/character_switcher.h"
#include "furnitures.h"
#include "item.h"
#include "item_helper.h"
#include "round.h"

constexpr int MAX_MAP_SIZE = 20;
constexpr int MAX_SEED_LENGTH = 20;

static void generate_and_insert_walls(std::string /* seed */) {
    // TODO generate walls based on seed
    const auto d_color = (Color){155, 75, 0, 255};
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
            if (i == 0 || j == 0 || i == MAX_MAP_SIZE - 1 ||
                j == MAX_MAP_SIZE - 1) {
                vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                std::shared_ptr<Wall> wall;
                wall.reset(new Wall(location, d_color));
                EntityHelper::addEntity(wall);
            }
        }
    }
}

static inline void lobby_generate_map(const std::string& seed,
                                      std::function<vec2()> get_rand_walkable) {
    log_info("{}, generating lobby map with seed ", is_server());
    {
        std::shared_ptr<CharacterSwitcher> charSwitch;
        const auto location = get_rand_walkable();
        charSwitch.reset(new CharacterSwitcher(location));
        EntityHelper::addEntity(charSwitch);
    }

    const auto generate_bag_box = [&]() {
        std::shared_ptr<BagBox> bagbox;
        const auto location = get_rand_walkable();
        bagbox.reset(new BagBox(location));
        EntityHelper::addEntity(bagbox);
    };

    generate_and_insert_walls(seed);
    generate_bag_box();

    log_info("{} finished generating lobby with {} entities", is_server(),
             EntityHelper::get_entities().size());
}

static inline void game_generate_map(const std::string& seed,
                                     std::function<vec2()> get_rand_walkable) {
    log_info("{} generating game map with seed {}", is_server(), seed);
    auto generate_tables = [&]() {
        {
            std::shared_ptr<Table> table;
            const auto location = get_rand_walkable();
            table.reset(new Table(location));
            EntityHelper::addEntity(table);

            std::shared_ptr<Pill> item;
            item.reset(new Pill(location, Color{255, 15, 240, 255}));
            ItemHelper::addItem(item);
            table->held_item = item;
        }

        {
            std::shared_ptr<Table> table;
            const auto location = get_rand_walkable();
            table.reset(new Table(location));
            EntityHelper::addEntity(table);

            std::shared_ptr<PillBottle> item;
            item.reset(new PillBottle(location, RED));
            ItemHelper::addItem(item);
            table->held_item = item;
        }
    };

    const auto generate_medicine_cabinet = [&]() {
        std::shared_ptr<MedicineCabinet> medicineCab;
        const auto location = get_rand_walkable();
        medicineCab.reset(new MedicineCabinet(location));
        EntityHelper::addEntity(medicineCab);
    };

    const auto generate_bag_box = [&]() {
        std::shared_ptr<BagBox> bagbox;
        const auto location = get_rand_walkable();
        bagbox.reset(new BagBox(location));
        EntityHelper::addEntity(bagbox);
    };

    const auto generate_register = [&]() {
        std::shared_ptr<Register> reg;
        const auto location = get_rand_walkable();
        reg.reset(new Register(location));
        EntityHelper::addEntity(reg);
    };

    // TODO replace with a CustomerSpawner eventually
    const auto generate_customer = []() {
        const auto location = vec2{-10 * TILESIZE, -10 * TILESIZE};
        std::shared_ptr<Customer> customer;
        customer.reset(new Customer(location, RED));
        EntityHelper::addEntity(customer);
    };

    auto generate_test = [&]() {
        for (int i = 0; i < 5; i++) {
            const auto location = get_rand_walkable();
            std::shared_ptr<Conveyer> conveyer;

            if (i == 0)
                conveyer.reset(new Conveyer(location));
            else
                conveyer.reset(new Grabber(location));

            EntityHelper::addEntity(conveyer);
        }
    };

    generate_and_insert_walls(seed);
    generate_tables();
    generate_tables();
    generate_medicine_cabinet();
    generate_bag_box();

    generate_register();
    generate_customer();
    generate_test();

    EntityHelper::invalidatePathCache();
}

struct LevelInfo {
    enum struct GeneratorType {
        Lobby,
        Game,
    } gen_type = GeneratorType::Lobby;

    //
    std::string seed;
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    std::optional<Round> active_round;

    Entities entities;
    Entities::size_type num_entities;

    Items items;
    Items::size_type num_items;

    LevelInfo() {}

    virtual void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        for (auto e : EntityHelper::get_entities()) {
            e->update(dt);
        }

        if (active_round.has_value()) {
            active_round->onUpdate(dt);
        }
    }

    virtual void onDraw(float) const {
        TRACY_ZONE_SCOPED;
        for (auto e : entities) {
            if (e) e->render();
            if (!e) log_warn("We have invalid entities");
        }

        for (auto i : items) {
            if (i) i->render();
            if (!i) log_warn("we have invalid items");
        }

        // TODO we want this to only be in lobby, how do we handle this?
        auto cam = GLOBALS.get_ptr<GameCam>("game_cam");
        if (cam) {
            raylib::DrawBillboard(cam->camera,
                                  TextureLibrary::get().get("face"),
                                  {
                                      1.f,
                                      0.f,
                                      1.f,
                                  },
                                  TILESIZE, WHITE);
        }
    }

    virtual void onDrawUI(float) {
        if (active_round.has_value()) {
            active_round->onDraw();
        }
    }

    auto get_rand_walkable() {
        vec2 location;
        do {
            location =
                vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
        } while (!EntityHelper::isWalkable(location));
        return location;
    }

    void update_seed(const std::string& s) {
        seed = s;
        //
        hashed_seed = hashString(seed);
        generator = make_engine(hashed_seed);
        // TODO leaving at 1 because we dont have a door to block the entrance
        dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);

        // TODO need to regenerate the map and clean up entitiyhelper
        client_entities_DO_NOT_USE.clear();
        client_items_DO_NOT_USE.clear();
        server_entities_DO_NOT_USE.clear();
        server_items_DO_NOT_USE.clear();

        switch (gen_type) {
            case GeneratorType::Lobby:
                return lobby_generate_map(
                    seed, std::bind(&LevelInfo::get_rand_walkable, this));
            case GeneratorType::Game:
                active_round = Round(120.f);
                return game_generate_map(
                    seed, std::bind(&LevelInfo::get_rand_walkable, this));
        }
    }

    void grab_things() {
        {
            entities.clear();
            EntityHelper::cleanup();
            auto es = EntityHelper::get_entities();
            this->entities = es;
            num_entities = this->entities.size();
        }

        {
            items.clear();
            auto is = ItemHelper::get_items();
            this->items = is;
            num_items = this->items.size();
        }
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value8b(num_entities);
        s.container(entities, num_entities,
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });

        s.value8b(num_items);
        s.container(items, num_items, [](S& s2, std::shared_ptr<Item>& item) {
            s2.ext(item, bitsery::ext::StdSmartPtr{});
        });
        s.text1b(seed, MAX_SEED_LENGTH);
        s.ext(active_round, bitsery::ext::StdOptional{});
    }
};

inline bool operator==(const LevelInfo& lhs, const LevelInfo& rhs) {
    return lhs.seed == rhs.seed;
}

inline bool operator!=(const LevelInfo& lhs, const LevelInfo& rhs) {
    return !(lhs == rhs);
}
