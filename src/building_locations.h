

#pragma once

#include <array>

#include "external_include.h"
#include "raylib.h"

enum BuildingType {
    ModelTest,
    Lobby,
    Store,
    Progression,
    Bar,
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

    auto& add_door(const DoorLocation& location) {
        switch (location) {
            case DoorLocation::None:
                break;
            case DoorLocation::Top: {
                doors.push_back({center().x, area.y});
                doors.push_back({center().x - 1, area.y});
                doors.push_back({center().x + 1, area.y});
                vomit_location = {center().x, area.y - 1};
            } break;
            case DoorLocation::Bottom: {
                doors.push_back({center().x, area.y + area.height - 1});
                doors.push_back({center().x - 1, area.y + area.height - 1});
                doors.push_back({center().x + 1, area.y + area.height - 1});
                vomit_location = {center().x, area.y + area.height + 1};
            } break;
            case DoorLocation::Left: {
                doors.push_back({area.x, center().y});
                doors.push_back({area.x, center().y - 1});
                doors.push_back({area.x, center().y + 1});
                vomit_location = {area.x - 1, center().y};
            } break;
            case DoorLocation::Right: {
                doors.push_back({area.x + area.width - 1, center().y});
                doors.push_back({area.x + area.width - 1, center().y - 1});
                doors.push_back({area.x + area.width - 1, center().y + 1});
                vomit_location = {area.x + area.width + 1, center().y};
            } break;
        }
        return *this;
    }

    bool is_inside(const vec2 pos) const {
        if (pos.x > max().x - 1 || pos.x < min().x) return false;
        if (pos.y > max().y - 1 || pos.y < min().y) return false;
        return true;
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
        .set_area({14.f, -35.f, 20.f, 25.f})
        .add_door(Building::DoorLocation::Bottom);
const Building STORE_BUILDING =  //
    Building()
        .set_area({-18.f, -35.f, 20.f, 25.f})
        .add_door(Building::DoorLocation::Bottom);
const Building BAR_BUILDING =  //
    Building()                 //
        .set_area({-25.f, -5.f, 30.f, 30.f});
