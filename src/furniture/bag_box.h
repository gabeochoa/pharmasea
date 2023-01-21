
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
    explicit BagBox(vec2 pos) : ItemContainer<Bag>(pos) { update_model(); }

    void update_model() {
        // log_info("model index: {}", model_index);
        // TODO add a component for this
        const bool in_planning = GameState::get().is(game::State::Planning);
        get<ModelRenderer>().update(ModelInfo{
            .model_name = in_planning ? "box" : "open_box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
};
