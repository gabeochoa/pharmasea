
#pragma once

#include "../item.h"
#include "base_component.h"

struct SimpleColoredBoxRenderer : public BaseComponent {
    virtual ~SimpleColoredBoxRenderer() {}

    Color face_color;
    Color base_color;

    void init(Color face, Color base) {
        face_color = face;
        base_color = base;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(face_color);
        s.object(base_color);
    }
};
