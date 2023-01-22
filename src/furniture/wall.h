
#pragma once

#include "../drawing_util.h"
#include "../external_include.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../furniture.h"

struct Wall : public Furniture {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Furniture>{});
    }

   public:
    enum Type {
        FULL,
        HALF,
        QUARTER,
        CORNER,
        TEE,
        DOUBLE_TEE,
    };

    Type type = FULL;

    Wall() : Furniture() {}
    Wall(vec3 p, Color c) : Furniture(p, c) {}
    Wall(vec2 p, Color c) : Furniture(p, c) {}

    virtual vec3 size() const override {
        return {TILESIZE, TILESIZE, TILESIZE};
    }

    // virtual void render_normal() const override {
    // TODO fix
    // switch (this->type) {
    // case Type::DOUBLE_TEE: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::FULL: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::HALF: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::CORNER: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::TEE: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::QUARTER:
    // break;
    // }
    // }

    virtual bool can_place_item_into(std::shared_ptr<Item>) override {
        return false;
    }
};
typedef Wall::Type WallType;
