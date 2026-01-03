
#pragma once

#include "../engine/util.h"
#include "../std_include.h"
#include "../vec_util.h"
#include "../vendor_include.h"
//
#include "base_component.h"

// TODO at some point id like to support diagonal angles
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

    int roundToNearest45(int degrees) const {
        int remainder = degrees % 45;
        int roundedValue = degrees - remainder;

        if (remainder >= 23)
            roundedValue += 45;
        else if (remainder <= -23)
            roundedValue -= 45;

        if (roundedValue >= 360) {
            roundedValue = roundedValue % 360;
        }
        while (roundedValue < 0) {
            roundedValue += 360;
        }

        return roundedValue;
    }

    FrontFaceDirection offsetFaceDirection(FrontFaceDirection startingDirection,
                                           int offset) const {
        const auto degreesOffset = roundToNearest45(
            (static_cast<int>(FrontFaceDirectionMap.at(startingDirection)) +
             offset));

        return DirectionToFrontFaceMap.at(degreesOffset % 360);
    }

    auto& init(vec3 pos, vec3 sz) {
        raw_position = pos;
        position = pos;

        _size = sz;
        return *this;
    }

    auto& update(vec3 npos) {
        this->raw_position = npos;
        sync();
        return *this;
    }
    auto& update_x(float x) {
        this->raw_position.x = x;
        sync();
        return *this;
    }
    auto& update_y(float y) {
        this->raw_position.y = y;
        sync();
        return *this;
    }
    auto& update_z(float z) {
        this->raw_position.z = z;
        sync();
        return *this;
    }

    auto& update_visual_offset(vec3 pos) {
        this->visual_offset = pos;
        sync();
        return *this;
    }

    void update_face_direction(float ang) { facing = ang; }

    [[nodiscard]] vec2 as2() const { return vec::to2(this->position); }

    [[nodiscard]] vec3 raw() const { return this->raw_position; }
    [[nodiscard]] vec3 pos() const { return this->position; }

    [[nodiscard]] vec3 size() const { return this->_size; }
    [[nodiscard]] float sizex() const { return this->size().x; }
    [[nodiscard]] float sizey() const { return this->size().y; }
    [[nodiscard]] float sizez() const { return this->size().z; }

    [[nodiscard]] vec3 viz_offset() const { return this->visual_offset; }
    [[nodiscard]] float viz_x() const { return this->visual_offset.x; }
    [[nodiscard]] float viz_y() const { return this->visual_offset.y; }
    [[nodiscard]] float viz_z() const { return this->visual_offset.z; }

    auto& update_size(vec3 sze) {
        this->_size = sze;
        return *this;
    }

    [[nodiscard]] BoundingBox raw_bounds() const {
        return get_bounds(this->raw_position, this->size());
    }

    [[nodiscard]] BoundingBox expanded_bounds(vec3 inc) const {
        return get_bounds(this->raw_position, this->size() + inc);
    }

    /*
     * Get the bounding box for this entity
     * @returns BoundingBox the box
     * */
    [[nodiscard]] BoundingBox bounds() const {
        return get_bounds(this->position, this->size());
    }

    [[nodiscard]] vec3 snap_position() const {
        return vec::snap(this->raw_position);
    }

    /*
     * Given another bounding box, check if it collides with this entity
     *
     * @param BoundingBox the box to test
     * */
    bool collides(BoundingBox b) const {
        // TODO fix move to collision component
        return CheckCollisionBoxes(bounds(), b);
    }

    Rectangle rectangular_bounds() const {
        return rectangular_bounds(pos(), size());
    }

    static Rectangle rectangular_bounds(vec3 pos, vec3 size) {
        return Rectangle{pos.x, pos.z, size.x, size.z};
    }

    vec3 circular_bounds() const { return circular_bounds(pos(), size()); }

    static vec3 circular_bounds(vec3 pos, vec3 size) {
        float radius = fmax(size.x * 0.6f, size.z * 0.6f);
        return vec3{pos.x, pos.z, radius};
    }

    /*
     * Rotate the facing direction of this entity, clockwise 90 degrees
     * */
    void rotate_facing_clockwise(int angle = 90) { facing += angle; }

    vec2 tile_directly_infront() const { return tile_infront(1); }

    /*
     * Returns the location of the tile `distance` distance in front of the
     * entity
     *
     * @param entity, the entity to get the spot in front of
     * @param int, how far in front to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    vec2 tile_infront(int distance) const {
        vec2 tile = vec::to2(snap_position());
        return tile_infront_given_pos(tile, distance, face_direction());
    }

    vec2 tile_behind(int distance) const {
        vec2 tile = vec::to2(snap_position());
        return tile_infront_given_pos(
            tile, distance, offsetFaceDirection(face_direction(), 180));
    }
    vec2 tile_behind(float distance) const {
        vec2 tile = vec::to2(snap_position());
        return tile_infront_given_pos(
            tile, distance, offsetFaceDirection(face_direction(), 180));
    }

    /*
     * Given a tile, distance, and direction, returns the location of the
     * tile `distance` distance in front of the tile
     *
     * @param vec2, the starting location
     * @param int, how far in front to go
     * @param Transform::FrontFaceDirection, which direction to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    static vec2 tile_infront_given_pos(vec2 tile, float distance,
                                       FrontFaceDirection direction) {
        // {0.3, 4.08}, 1, Transform::Forward
        // {0.3, ceil(4.08 + 1)} => ceil(5.08) => {0.3, 6}

        if (direction & Transform::FORWARD) {
            tile.y = ceil(tile.y);
            tile.y += distance * TILESIZE;
        }

        if (direction & Transform::BACK) {
            tile.y = floor(tile.y);
            tile.y -= distance * TILESIZE;
        }

        if (direction & Transform::RIGHT) {
            tile.x = ceil(tile.x);
            tile.x += distance * TILESIZE;
        }

        if (direction & Transform::LEFT) {
            tile.x = floor(tile.x);
            tile.x -= distance * TILESIZE;
        }
        return tile;
    }
    static vec2 tile_infront_given_pos(vec2 tile, int dist,
                                       FrontFaceDirection direction) {
        float distance = static_cast<float>(dist);
        return tile_infront_given_pos(tile, distance, direction);
    }

    void turn_to_face_pos(const vec2 goal) {
        float theta_rad = atan2(as2().y - goal.y, as2().x - goal.x);
        float theta_deg = util::rad2deg(theta_rad);
        // TODO replace with fmod
        float turn_degrees = static_cast<float>((180 - (int) theta_deg) % 360);
        int to_rotate = static_cast<int>((turn_degrees - facing) + 90);
        rotate_facing_clockwise(to_rotate);
    }

    void trunc(int places) {
        update({
            util::trunc(raw().x, places),
            util::trunc(raw().y, places),
            util::trunc(raw().z, places),
        });
    }

    [[nodiscard]] const FrontFaceDirection& face_direction() const {
        const auto r = roundToNearest45(static_cast<int>(facing));
        return DirectionToFrontFaceMap.at(r);
    }

    // This exists so that its easy to make sure the real location
    // matches the preview location
    [[nodiscard]] vec3 drop_location() const {
        return vec::snap(vec::to3(tile_directly_infront()));
    }

    // TODO private
    float facing = 0.f;

   private:
    vec2 get_heading() const {
        const float target_facing_ang = util::deg2rad(facing);
        return vec2{
            cosf(target_facing_ang),
            sinf(target_facing_ang),
        };
    }

    void sync() {
        // TODO neeed to add a component for snappable then we can just do
        // if(parent->has<IsSnappedToGrid>()){
        //  snap();
        // }
        this->position = this->raw_position;
    }

    vec3 _size = {TILESIZE, TILESIZE, TILESIZE};
    vec3 position = {0, 0, 0};
    vec3 raw_position = {0, 0, 0};
    vec3 visual_offset = {0, 0, 0};

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.visual_offset,              //
            self.raw_position,               //
            self.position,                   //
            self.facing,                     //
            self._size                       //
        );
    }
};

inline std::ostream& operator<<(std::ostream& os, const Transform& t) {
    os << "Transform<> " << t.pos() << " " << t.size();
    return os;
}
