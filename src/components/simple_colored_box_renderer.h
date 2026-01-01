
#pragma once

#include "base_component.h"

struct SimpleColoredBoxRenderer : public BaseComponent {
    auto& update_face(Color face) {
        face_color = face;
        return *this;
    }
    auto& update_base(Color base) {
        base_color = base;
        return *this;
    }

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
