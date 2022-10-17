
#pragma once 

#include "../external_include.h"
#include "../drawing_util.h"
//
#include "../entity.h"
#include "../globals.h"
//
#include "../furniture.h"

struct Wall : public Furniture {
    enum Type {
        FULL,
        HALF,
        QUARTER,
        CORNER,
        TEE,
        DOUBLE_TEE,
    };

    Type type = FULL;

    Wall(vec3 p, Color c) : Furniture(p, c) {}
    Wall(vec2 p, Color c) : Furniture(p, c) {}

    virtual vec3 size() const override {
        return {TILESIZE, TILESIZE, TILESIZE};
    }

    virtual void render() const override {
        switch (this->type) {
            case Type::DOUBLE_TEE: {
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::FULL: {
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::HALF: {
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::CORNER: {
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::TEE: {
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::QUARTER:
                break;
        }
    }

    void update(float) override {}
};
typedef Wall::Type WallType;
