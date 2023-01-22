
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
        : Furniture(pos, ui::color::brown, ui::color::brown) {
        get<HasWork>().init([this](std::shared_ptr<Person> person, float dt) {
            const float amt = 2.f;
            HasWork& hasWork = this->get<HasWork>();
            hasWork.pct_work_complete += amt * dt;
            if (hasWork.pct_work_complete >= 1.f) {
                hasWork.pct_work_complete = 0.f;
                person->select_next_character_model();
            }
        });
    }
};
