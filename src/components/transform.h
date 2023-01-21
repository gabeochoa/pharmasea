
#pragma once

#include "../std_include.h"
#include "../vec_util.h"
#include "../vendor_include.h"
//
#include "base_component.h"

struct Transform : public BaseComponent {
    enum FrontFaceDirection {
        FORWARD = 0x1,
        RIGHT = 0x2,
        BACK = 0x4,
        LEFT = 0x8
    };

    const std::map<FrontFaceDirection, float> FrontFaceDirectionMap{
        {FORWARD, 0.0f},        {FORWARD | RIGHT, 45.0f}, {RIGHT, 90.0f},
        {BACK | RIGHT, 135.0f}, {BACK, 180.0f},           {BACK | LEFT, 225.0f},
        {LEFT, 270.0f},         {FORWARD | LEFT, 315.0f}};

    const std::map<int, FrontFaceDirection> DirectionToFrontFaceMap{
        {0, FORWARD},        {45, FORWARD | RIGHT}, {90, RIGHT},
        {135, BACK | RIGHT}, {180, BACK},           {225, BACK | LEFT},
        {270, LEFT},         {315, FORWARD | LEFT}};

    FrontFaceDirection offsetFaceDirection(FrontFaceDirection startingDirection,
                                           float offset) const {
        const auto degreesOffset =
            static_cast<int>(FrontFaceDirectionMap.at(startingDirection) +
                             static_cast<int>(offset));
        return DirectionToFrontFaceMap.at(degreesOffset % 360);
    }

    vec3 raw_position;
    vec3 prev_position;
    vec3 position;
    FrontFaceDirection face_direction = FrontFaceDirection::FORWARD;
    vec3 size;

    virtual ~Transform() {}

    void init(vec3 pos, vec3 sz) {
        raw_position = pos;
        prev_position = pos;
        position = pos;

        size = sz;
    }

    [[nodiscard]] vec2 as2() const { return vec::to2(this->position); }

    [[nodiscard]] virtual BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size);
    }

    /*
     * Get the bounding box for this entity
     * @returns BoundingBox the box
     * */
    [[nodiscard]] virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size / 2.0f);
    }

    [[nodiscard]] vec3 snap_position() const {
        return vec::snap(this->raw_position);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(raw_position);
        s.object(prev_position);
        s.object(position);
        s.value4b(face_direction);
        s.object(size);
    }
};
