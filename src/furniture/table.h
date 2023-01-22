
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../components/custom_item_position.h"
#include "../furniture.h"
#include "../person.h"

struct Table : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

    void add_static_components() {
        addComponent<CustomHeldItemPosition>().init(
            [](Transform& transform) -> vec3 {
                auto new_pos = transform.position;
                new_pos.y += TILESIZE / 2;
                return new_pos;
            });
    }

   public:
    Table() { add_static_components(); }
    explicit Table(vec2 pos)
        : Furniture(pos, ui::color::brown, ui::color::brown) {
        add_static_components();
        get<HasWork>().init([this](std::shared_ptr<Person>, float dt) {
            // TODO eventually we need it to decide whether it has work based on
            // the current held item
            const float amt = 0.5f;
            HasWork& hasWork = this->get<HasWork>();
            hasWork.pct_work_complete += amt * dt;
            if (hasWork.pct_work_complete >= 1.f)
                hasWork.pct_work_complete = 0.f;
        });
    }
};
