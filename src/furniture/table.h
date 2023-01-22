
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../components/custom_item_position.h"
#include "../furniture.h"

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
    }

    // TODO eventually we need it to decide whether it has work based on the
    // current held item
    virtual void do_work(float dt, std::shared_ptr<Person>) override {
        const float amt = 0.5f;
        pct_work_complete += amt * dt;
        if (pct_work_complete >= 1.f) pct_work_complete = 0.f;
    }

    // Does this piece of furniture have work to be done?
    virtual bool has_work() const override { return true; }
};
