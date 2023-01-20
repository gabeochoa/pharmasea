
#pragma once

#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct CharacterSwitcher : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    CharacterSwitcher() {}
    explicit CharacterSwitcher(vec2 pos)
        : Furniture(pos, ui::color::brown, ui::color::brown) {}

    virtual void do_work(float dt, Person* person) override {
        const float amt = 1.f;
        pct_work_complete += amt * dt;
        if (pct_work_complete >= 1.f) {
            pct_work_complete = 0.f;
            person->select_next_character_model();
        }
    }

    // Does this piece of furniture have work to be done?
    virtual bool has_work() const override { return true; }

    virtual void update_held_item_position() override {
        if (held_item != nullptr) {
            auto new_pos = this->position;
            new_pos.y += TILESIZE / 2;
            held_item->update_position(new_pos);
        }
    }
};
