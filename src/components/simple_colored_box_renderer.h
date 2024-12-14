
#pragma once

#include "base_component.h"

struct SimpleColoredBoxRenderer : public BaseComponent {
    virtual ~SimpleColoredBoxRenderer() {}

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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                face_color, base_color);
    }
};

CEREAL_REGISTER_TYPE(SimpleColoredBoxRenderer);
