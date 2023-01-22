
#pragma once

#include "drawing_util.h"
#include "external_include.h"
//
#include "entity.h"
#include "globals.h"
#include "person.h"

struct Furniture : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
        // Only need to serialize things that are needed for render
        s.value4b(pct_work_complete);
    }

   protected:
    Furniture() : Entity() {}

    float pct_work_complete = 0.f;

   public:
    Furniture(vec2 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec3 pos, Color face_color_in)
        : Entity(pos, face_color_in, face_color_in) {}
    Furniture(vec2 pos, Color face_color_in, Color base_color_in)
        : Entity(pos, face_color_in, base_color_in) {}

    virtual void update_held_item_position() override {
        // TODO
        // if (held_item() == nullptr) return;
        // auto new_pos = this->get<Transform>().position;
        // new_pos.y += TILESIZE / 4;
        // held_item()->update_position(new_pos);
    }

    virtual void render_normal() const override {
        Entity::render_normal();
        render_progress_bar();
    }

    void render_progress_bar() const {
        if (pct_work_complete <= 0.01f || !has_work()) return;

        const int length = 20;
        const int full = (int) (pct_work_complete * length);
        const int empty = length - full;
        auto progress =
            fmt::format("[{:=>{}}{: >{}}]", "", full, "", empty * 2);
        DrawFloatingText(
            this->get<Transform>().raw_position + vec3({0, TILESIZE, 0}),
            Preload::get().font, progress.c_str());

        // TODO eventually add real rectangle progress bar
        // auto game_cam = GLOBALS.get<GameCam>("game_cam");
        // DrawBillboardRec(game_cam.camera, TextureLibrary::get().get("face"),
        // Rectangle({0, 0, 260, 260}),
        // this->raw_position + vec3({-(pct_complete * TILESIZE),
        // TILESIZE * 2, 0}),
        // {pct_complete * TILESIZE, TILESIZE}, WHITE);
    }

    // Note: do nothing by default
    virtual void do_work(float, std::shared_ptr<Person>) {}

    // Does this piece of furniture have work to be done?
    virtual bool has_work() const { return false; }

    // TODO this should be const
    virtual bool add_to_navmesh() override { return true; }
    virtual bool can_rotate() const { return true; }
    // TODO this should be const
    virtual bool can_be_picked_up() { return !this->is_held; }
    // TODO this should be const
    virtual bool can_place_item_into(std::shared_ptr<Item> = nullptr) override {
        // TODO this should be a separate component
        return get<CanHoldItem>().empty();
    }

    virtual bool has_held_item() const {
        return get<CanHoldItem>().is_holding_item();
    }
};
