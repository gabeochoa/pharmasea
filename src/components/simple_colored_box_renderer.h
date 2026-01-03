
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

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.face_color,                //
            self.base_color                 //
        );
    }
};
