
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"
#include "../menu.h"

struct MedicineCabinet : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    MedicineCabinet() {}
    explicit MedicineCabinet(vec2 pos) : Furniture(pos, WHITE, WHITE) {}

    // TODO im thinkin we can do something like ItemBox<Bag> and just support
    // them dynamically
    virtual bool can_place_item_into(
        std::shared_ptr<Item> item = nullptr) override {
        auto pill_bottle = dynamic_pointer_cast<PillBottle>(item);
        if (!pill_bottle) return false;
        if (!pill_bottle->empty()) return false;
        return true;
    }

    virtual void game_update(float dt) override {
        Furniture::game_update(dt);
        if (this->held_item == nullptr) {
            this->held_item.reset(
                new PillBottle(this->position, (Color){255, 15, 240, 255}));
            ItemHelper::addItem(this->held_item);
        }
    }

    virtual std::optional<ModelInfo> model() const override {
        return ModelInfo{
            .model = ModelLibrary::get().get("medicine_cabinet"),
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        };
    }
};
