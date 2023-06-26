
#pragma once

#include "../engine/util.h"
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

    /*
     * Rotate the facing direction of this entity, clockwise 90 degrees
     * */
    void rotate_facing_clockwise(int angle = 90) {
        update_face_direction(offsetFaceDirection(face_direction(), angle));
    }

    /*
     * Returns the location of the tile `distance` distance in front of the
     * entity
     *
     * @param entity, the entity to get the spot in front of
     * @param int, how far in front to go
     *
     * @returns vec2 the location `distance` tiles ahead
     * */
    vec2 tile_infront_given_player(int distance) {
        vec2 tile = vec::to2(snap_position());
        return tile_infront_given_pos(tile, distance, face_direction());
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
    static vec2 tile_infront_given_pos(
        vec2 tile, int distance,
        Transform::Transform::FrontFaceDirection direction) {
        if (direction & Transform::FORWARD) {
            tile.y += distance * TILESIZE;
            tile.y = ceil(tile.y);
        }
        if (direction & Transform::BACK) {
            tile.y -= distance * TILESIZE;
            tile.y = floor(tile.y);
        }

        if (direction & Transform::RIGHT) {
            tile.x += distance * TILESIZE;
            tile.x = ceil(tile.x);
        }
        if (direction & Transform::LEFT) {
            tile.x -= distance * TILESIZE;
            tile.x = floor(tile.x);
        }
        return tile;
    }

    void turn_to_face_entity(Transform target) {
        // dot product visualizer https://www.falstad.com/dotproduct/

        // the angle between two vecs is
        // @ = arccos(  (a dot b) / ( |a| * |b| ) )

        // first get the headings and normalise so |x| = 1
        const vec2 my_heading = vec::norm(this->get_heading());
        const vec2 tar_heading = vec::norm(target.get_heading());
        // dp = ( (a dot b )/ 1)
        float dot_product = vec::dot2(my_heading, tar_heading);
        // arccos(dp)
        float theta_rad = acosf(dot_product);
        float theta_deg = util::rad2deg(theta_rad);
        int turn_degrees = (180 - (int) theta_deg) % 360;
        // TODO fix entity
        (void) turn_degrees;
        if (turn_degrees > 0 && turn_degrees <= 45) {
            update_face_direction(
                static_cast<Transform::FrontFaceDirection>(0));
        } else if (turn_degrees > 45 && turn_degrees <= 135) {
            update_face_direction(
                static_cast<Transform::FrontFaceDirection>(90));
        } else if (turn_degrees > 135 && turn_degrees <= 225) {
            update_face_direction(
                static_cast<Transform::FrontFaceDirection>(180));
        } else if (turn_degrees > 225) {
            update_face_direction(
                static_cast<Transform::FrontFaceDirection>(270));
        }
    }

   private:
    vec2 get_heading() {
        const float target_facing_ang =
            util::deg2rad(FrontFaceDirectionMap.at(face_direction()));
        return vec2{
            cosf(target_facing_ang),
            sinf(target_facing_ang),
        };
    }

    void sync() {
        // TODO is there a way for us to figure out if this
        // is a snappable entity?
        this->position = this->raw_position;
    }

    FrontFaceDirection face = FrontFaceDirection::FORWARD;
    vec3 _size = {TILESIZE, TILESIZE, TILESIZE};
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

inline std::ostream& operator<<(std::ostream& os, const Transform& t) {
    os << "Transform<> " << t.pos() << " " << t.size();
    return os;
}
