
#pragma once

#include "engine/globals_register.h"
#include "engine/ui_color.h"
#include "entity.h"
#include "entityhelper.h"
#include "furniture.h"
#include "item.h"
#include "item_helper.h"
#include "round.h"
#include "system/system_manager.h"

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
                std::shared_ptr<Furniture> wall;
                wall.reset(Furniture::make_wall(location, d_color));
                EntityHelper::addEntity(wall);
            }
        }
    }
}

struct LevelInfo {
    bool was_generated = false;

    SystemManager system_manager;

    Entities entities;
    Entities::size_type num_entities;

    Items items;
    Items::size_type num_items;

    std::string seed;

    virtual void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        system_manager.update(dt);
    }

    virtual void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;

        system_manager.render(dt);

        for (auto i : items) {
            if (i) i->render();
            if (!i) log_warn("we have invalid items");
        }
    }

    virtual void onDrawUI(float) {}

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

    void ensure_generated_map(const std::string& new_seed) {
        if (was_generated) return;
        seed = new_seed;
        was_generated = true;
        generate_map();
    }

    virtual void generate_map() = 0;

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
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);
    }
};

struct LobbyMapInfo : public LevelInfo {
    virtual void generate_map() override {
        {
            std::shared_ptr<Furniture> charSwitch;
            const auto location = vec2{5, 5};
            charSwitch.reset(Furniture::make_character_switcher(location));
            EntityHelper::addEntity(charSwitch);
        }
    }

    virtual void onDraw(float dt) const override {
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

        LevelInfo::onDraw(dt);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<LevelInfo>{});
    }
};

struct GameMapInfo : public LevelInfo {
    //
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    std::optional<Round> active_round;

    void update_seed(const std::string& s) {
        seed = s;
        hashed_seed = hashString(seed);
        generator = make_engine(hashed_seed);
        // TODO leaving at 1 because we dont have a door to block the entrance
        dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);

        active_round = Round(120.f);

        // TODO need to regenerate the map and clean up entitiyhelper
    }

    virtual void onUpdate(float dt) override {
        // log_info("update game map info");
        LevelInfo::onUpdate(dt);
        if (active_round.has_value()) {
            active_round->onUpdate(dt);
        }
    }

    virtual void onDrawUI(float) override {
        if (active_round.has_value()) {
            active_round->onDraw();
        }
    }

   private:
    auto get_rand_walkable() {
        vec2 location;
        do {
            location =
                vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
        } while (!EntityHelper::isWalkable(location));
        return location;
    }

    virtual void generate_map() override {
        server_entities_DO_NOT_USE.clear();
        server_items_DO_NOT_USE.clear();

        auto generate_tables = [this]() {
            {
                const auto location = get_rand_walkable();

                std::shared_ptr<Furniture> table;
                table.reset(Furniture::make_table(location));
                EntityHelper::addEntity(table);

                std::shared_ptr<Pill> item;
                item.reset(new Pill(location, Color{255, 15, 240, 255}));
                ItemHelper::addItem(item);
                table->get<CanHoldItem>().update(item);
            }

            {
                const auto location = get_rand_walkable();

                std::shared_ptr<Furniture> table;
                table.reset(Furniture::make_table(location));
                EntityHelper::addEntity(table);

                std::shared_ptr<PillBottle> item;
                item.reset(new PillBottle(location, RED));
                ItemHelper::addItem(item);
                table->get<CanHoldItem>().update(item);
            }
        };

        const auto generate_medicine_cabinet = [this]() {
            std::shared_ptr<Furniture> medicineCab;
            const auto location = get_rand_walkable();
            medicineCab.reset(Furniture::make_medicine_cabinet(location));
            EntityHelper::addEntity(medicineCab);
        };

        const auto generate_bag_box = [this]() {
            std::shared_ptr<Furniture> bagbox;
            const auto location = get_rand_walkable();
            bagbox.reset(Furniture::make_bagbox(location));
            EntityHelper::addEntity(bagbox);
        };

        const auto generate_register = [this]() {
            std::shared_ptr<Furniture> reg;
            const auto location = get_rand_walkable();
            reg.reset(Furniture::make_register(location));
            EntityHelper::addEntity(reg);
        };

        // TODO replace with a CustomerSpawner eventually
        const auto generate_customer = []() {
            const auto location = vec2{-10 * TILESIZE, -10 * TILESIZE};
            std::shared_ptr<Customer> customer;
            customer.reset(new Customer(vec::to3(location), RED));
            EntityHelper::addEntity(dynamic_pointer_cast<Entity>(customer));
        };

        auto generate_test = [this]() {
            for (int i = 0; i < 5; i++) {
                const auto location = get_rand_walkable();
                std::shared_ptr<Furniture> conveyer;

                if (i == 0)
                    conveyer.reset(Furniture::make_conveyer(location));
                else
                    conveyer.reset(Furniture::make_grabber(location));

                EntityHelper::addEntity(conveyer);
            }
        };

        generate_and_insert_walls(this->seed);
        generate_tables();
        generate_tables();
        generate_medicine_cabinet();
        generate_bag_box();

        generate_register();
        generate_customer();
        generate_test();

        EntityHelper::invalidatePathCache();
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<LevelInfo>{});
        s.ext(active_round, bitsery::ext::StdOptional{});
        s.value8b(hashed_seed);
        // TODO these arent serializable...
        // s.object(generator);
        // s.object(dist);
    }
};
