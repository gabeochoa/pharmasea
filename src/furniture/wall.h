
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

    Wall(raylib::vec3 p, raylib::Color c) : Furniture(p, c) {}
    Wall(raylib::vec2 p, raylib::Color c) : Furniture(p, c) {}

    virtual raylib::vec3 size() const override {
        return {TILESIZE, TILESIZE, TILESIZE};
    }

    virtual void render() const override {
        switch (this->type) {
            case Type::DOUBLE_TEE: {
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::FULL: {
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::HALF: {
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::CORNER: {
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x,                      //
                               this->size().y,                      //
                               this->size().z / 2,                  //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
            } break;
            case Type::TEE: {
                raylib::DrawCubeCustom(this->raw_position,                  //
                               this->size().x / 2,                  //
                               this->size().y,                      //
                               this->size().z,                      //
                               FrontFaceDirectionMap.at(face_direction),  //
                               this->face_color, this->base_color);
                raylib::DrawCubeCustom(this->raw_position,                  //
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
