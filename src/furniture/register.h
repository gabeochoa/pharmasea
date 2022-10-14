
#pragma once 

#include "../external_include.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../furniture.h"


struct Register : public Furniture {

    Register(vec2 pos): Furniture(pos, BLACK, DARKGRAY) {}

    virtual bool can_rotate() override {
        return true;
    }

    virtual bool can_be_picked_up() override {
        return true;
    }

};
