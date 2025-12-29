

#pragma once

#include <array>

#include "external_include.h"

enum BuildingType {
    ModelTest,
    Lobby,
    Store,
    Progression,
    Bar,
    LoadSave,
};

struct Building {
    Rectangle area;
    BoundingBox bounds;

    enum struct DoorLocation {
        None,
        Top,
        Bottom,
        Left,
        Right,
        BottomRight,
    } door_location;
    std::vector<vec2> doors;

    // where we should spit you out
    vec2 vomit_location;

    vec2 center() const {
        return vec2{area.x + (area.width / 2.f), area.y + (area.height / 2.f)};
    }

    vec3 to3() const {
        auto center_ = center();
        return vec3{center_.x, 0, center_.y};
    }

    vec2 min() const { return vec2{area.x, area.y}; }
    vec2 max() const { return vec2{area.x + area.width, area.y + area.height}; }

    auto& set_area(Rectangle rect) {
        area = rect;
        bounds = BoundingBox{{rect.x, 0, rect.y},
                             {rect.x + rect.width, 0, rect.y + rect.height}};
        doors.clear();
        return *this;
    }

    Building& add_door(const DoorLocation& location);

    bool is_inside(const vec2 pos) const {
        if (pos.x > max().x - 1 || pos.x < min().x) return false;
        if (pos.y > max().y - 1 || pos.y < min().y) return false;
        return true;
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(area);
        s.object(bounds);
        s.value4b(door_location);
    }
};

const Building MODEL_TEST_BUILDING =  //
    Building()                        //
        .set_area({100.f, -50.f, 30.f, 50.f});

const Building LOBBY_BUILDING =  //
    Building()                   //
        .set_area({25.f, 5.f, 15.f, 15.f});
const Building PROGRESSION_BUILDING =
    Building()
        .set_area({12.f, -23.f, 20.f, 15.f})
        .add_door(Building::DoorLocation::Bottom);
const Building STORE_BUILDING =  //
    Building()
        .set_area({-14.f, -38.f, 18.f, 30.f})
        .add_door(Building::DoorLocation::BottomRight);
const Building BAR_BUILDING =  //
    Building()                 //
        .set_area({-25.f, -5.f, 29.f, 30.f})
        // Used for lighting spill direction/origin and (optionally) wall/door gen.
        // Pick a single, stable entrance until we make bar entrances data-driven.
        .add_door(Building::DoorLocation::Bottom);

// Diagetic "memory card" showroom.
const Building LOAD_SAVE_BUILDING =  //
    Building()                       //
        .set_area({150.f, -10.f, 30.f, 30.f});
