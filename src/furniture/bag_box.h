
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"
#include "../menu.h"

struct BagBox : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    BagBox() {}
    explicit BagBox(vec2 pos) : Furniture(pos, WHITE, WHITE) {}

    // TODO im thinkin we can do something like ItemBox<Bag> and just support
    // them dynamically
    virtual bool can_place_item_into(
        std::shared_ptr<Item> item = nullptr) override {
        auto bag = dynamic_pointer_cast<Bag>(item);
        if (!bag) return false;
        if (!bag->empty()) return false;
        return true;
    }

    virtual void game_update(float dt) override {
        Furniture::game_update(dt);
        if (this->held_item == nullptr) {
            this->held_item.reset(
                new Bag(this->position, (Color){255, 15, 240, 255}));
            ItemHelper::addItem(this->held_item);
        }
    }

    virtual std::optional<ModelInfo> model() const override {
        const bool in_planning = Menu::get().is(Menu::State::Planning);

        return ModelInfo{
            .model = in_planning ? ModelLibrary::get().get("box")
                                 : ModelLibrary::get().get("open_box"),
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        };
    }
};
