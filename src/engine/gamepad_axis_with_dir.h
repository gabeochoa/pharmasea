
#pragma once

#include "../vendor_include.h"

struct GamepadAxisWithDir {
    raylib::GamepadAxis axis;
    float dir = -1;

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(  //
            self.axis,   //
            self.dir     //
        );
    }
};
