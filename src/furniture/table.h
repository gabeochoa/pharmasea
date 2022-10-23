
#pragma once

#include "../external_include.h"
//
#include "../assert.h"
#include "../entity.h"
#include "../globals.h"
//
#include "../aiperson.h"
#include "../furniture.h"

struct Table : public Furniture {
    Table(raylib::vec2 pos) : Furniture(pos, ui::color::brown, ui::color::brown) {}

    virtual bool can_rotate() override { return true; }

    virtual bool can_be_picked_up() override { return true; }

    virtual bool can_place_item_into() override {
        return this->held_item == nullptr;
    }
};
