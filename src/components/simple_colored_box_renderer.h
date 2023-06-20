
#pragma once

#include "../item.h"
#include "base_component.h"

struct SimpleColoredBoxRenderer : public BaseComponent {
    virtual ~SimpleColoredBoxRenderer() {}

    void update(Color face, Color base) {
        face_color = face;
        base_color = base;
    }

    void update_face(Color face) { face_color = face; }
    void update_base(Color base) { base_color = base; }

    [[nodiscard]] Color face() { return face_color; }
    [[nodiscard]] Color base() { return base_color; }

   private:
    Color face_color;
    Color base_color;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(face_color);
        s.object(base_color);
    }
};
