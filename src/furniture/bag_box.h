
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
#include "../statemanager.h"
//
#include "item_container.h"

struct BagBox : public ItemContainer<Bag> {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    BagBox() {}
    explicit BagBox(vec2 pos) : ItemContainer<Bag>(pos) {}

    virtual std::optional<ModelInfo> model() const override {
        const bool in_planning = GameState::get().is(game::State::Planning);

        return ModelInfo{
            .model = in_planning ? ModelLibrary::get().get("box")
                                 : ModelLibrary::get().get("open_box"),
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        };
    }
};
