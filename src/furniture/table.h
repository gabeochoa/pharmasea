
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct Table : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    Table() {}
    explicit Table(vec2 pos)
        : Furniture(pos, ui::color::brown, ui::color::brown) {}

    // TODO eventually we need it to decide whether it has work based on the
    // current held item
    virtual void do_work(float dt, Person*) override {
        const float amt = 0.5f;
        pct_work_complete += amt * dt;
        if (pct_work_complete >= 1.f) pct_work_complete = 0.f;
    }

    // Does this piece of furniture have work to be done?
    virtual bool has_work() const override { return true; }

    virtual void update_held_item_position() override {
        if (held_item() != nullptr) {
            auto new_pos = this->get<Transform>().position;
            new_pos.y += TILESIZE / 2;
            held_item()->update_position(new_pos);
        }
    }
};
