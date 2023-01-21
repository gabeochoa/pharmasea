
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "item_container.h"

struct MedicineCabinet : public ItemContainer<PillBottle> {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<ItemContainer>{});
    }

   public:
    MedicineCabinet() {}
    explicit MedicineCabinet(vec2 pos) : ItemContainer<PillBottle>(pos) {
        update_model();
    }

    void update_model() {
        // TODO add a component for this
        get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
};
