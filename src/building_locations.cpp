
#include "building_locations.h"

Building& Building::add_door(const DoorLocation& location) {
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
        case DoorLocation::BottomRight:
            doors.push_back(
                {area.x + area.width - 3, area.y + area.height - 1});
            doors.push_back(
                {area.x + area.width - 4, area.y + area.height - 1});
            doors.push_back(
                {area.x + area.width - 5, area.y + area.height - 1});
            vomit_location = {area.x + area.width - 4, area.y + area.height};
            break;
    }
    return *this;
}
