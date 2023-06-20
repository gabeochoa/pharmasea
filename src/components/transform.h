
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

    virtual ~Transform() {}

    void init(vec3 pos, vec3 sz) {
        raw_position = pos;
        position = pos;

        _size = sz;
    }

    void update(vec3 npos) {
        this->raw_position = npos;
        sync();
    }
    void update_x(float x) {
        this->raw_position.x = x;
        sync();
    }
    void update_y(float y) {
        this->raw_position.y = y;
        sync();
    }
    void update_z(float z) {
        this->raw_position.z = z;
        sync();
    }

    void update_face_direction(FrontFaceDirection dir) { this->face = dir; }

    [[nodiscard]] FrontFaceDirection face_direction() const {
        return this->face;
    }
    [[nodiscard]] vec2 as2() const { return vec::to2(this->position); }

    [[nodiscard]] vec3 raw() const { return this->raw_position; }
    [[nodiscard]] vec3 pos() const { return this->position; }

    [[nodiscard]] vec3 size() const { return this->_size; }
    [[nodiscard]] float sizex() const { return this->size().x; }
    [[nodiscard]] float sizey() const { return this->size().y; }
    [[nodiscard]] float sizez() const { return this->size().z; }

    void update_size(vec3 sze) { this->_size = sze; }

    [[nodiscard]] virtual BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    /*
     * Get the bounding box for this entity
     * @returns BoundingBox the box
     * */
    [[nodiscard]] virtual BoundingBox bounds() const {
        return get_bounds(this->position, this->size() / 2.0f);
    }

    [[nodiscard]] vec3 snap_position() const {
        return vec::snap(this->raw_position);
    }

   private:
    void sync() {
        // TODO is there a way for us to figure out if this
        // is a snappable entity?
        this->position = this->raw_position;
    }

    FrontFaceDirection face = FrontFaceDirection::FORWARD;
    vec3 _size;
    vec3 position;
    vec3 raw_position;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.object(raw_position);
        s.object(position);
        s.value4b(face);
        s.object(_size);
    }
};

std::ostream& operator<<(std::ostream& os, const Transform& t) {
    os << "Transform<> " << t.pos() << " " << t.size();
    return os;
}
