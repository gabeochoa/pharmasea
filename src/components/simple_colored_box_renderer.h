
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

    [[nodiscard]] Color face() const { return face_color; }
    [[nodiscard]] Color base() const { return base_color; }

   private:
    Color face_color = PINK;
    Color base_color = PINK;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(face_color);
        s.object(base_color);
    }
};
