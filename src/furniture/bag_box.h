
#pragma once

#include "../external_include.h"
//
#include "../assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct BagBox : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    BagBox() {}
    BagBox(vec2 pos)
        : Furniture(pos, ui::color::tan_brown, ui::color::light_brown) {}
};
