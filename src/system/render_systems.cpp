#include "render_systems.h"

#include "../components/transform.h"
#include "../drawing_util.h"
#include "../engine/frustum.h"
#include "../engine/globals_register.h"
#include "../engine/ui/color.h"
#include "../entity_helper.h"
#include "../globals.h"
#include "../strings.h"
#include "rendering_system.h"
#include "system_manager.h"

namespace system_manager {

struct OnFrameStartSystem : public afterhours::System<> {
    static Frustum frustum;

    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) override {
#if LOG_RENDER_ENT_COUNT
        log_warn("num entities drawn: {}", num_ents_drawn);
        num_ents_drawn = 0;
#endif
        frustum.fetch_data();
    }

#if LOG_RENDER_ENT_COUNT
    static size_t num_ents_drawn;
#endif
};

struct RenderWalkableSpotsSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        return GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    }

    virtual void once(const float) override {
        // TODO For some reason this also triggers the walkable.contains
        // segfault
        return;

        if (!GLOBALS.get<bool>("debug_ui_enabled")) return;

        for (int i = -25; i < 25; i++) {
            for (int j = -25; j < 25; j++) {
                vec2 pos2 = {(float) i, (float) j};
                bool walkable = EntityHelper::isWalkable(pos2);
                if (walkable) continue;

                DrawCubeCustom(vec::to3(pos2), TILESIZE,
                               TILESIZE + TILESIZE / 10.f, TILESIZE, 0,
                               walkable ? ui::color::transleucent_green
                                        : ui::color::transleucent_red,
                               walkable ? ui::color::transleucent_green
                                        : ui::color::transleucent_red);
            }
        }
    }
};

struct RenderEntitySystem : public afterhours::System<Transform> {
    mutable bool debug_mode_on = false;

    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) const override {
        // Cache debug mode setting
        debug_mode_on = GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    }

    virtual void for_each_with(const Entity& entity, const Transform&,
                               float dt) const override {
        // -- this is a little fake culling
        // vec2 e_pos = entity.get<Transform>().as2();
        // if (vec::distance(e_pos,
        // vec::to2(cam.camera.position)) > 50.f) { return;
        // }

        // TODO fold this in later
        render_manager::render(entity, dt, debug_mode_on);
    }
};

Frustum OnFrameStartSystem::frustum;

#if LOG_RENDER_ENT_COUNT
size_t OnFrameStartSystem::num_ents_drawn = 0;
#endif

void register_render_systems(afterhours::SystemManager& systems) {
    systems.register_render_system(std::make_unique<OnFrameStartSystem>());
    systems.register_render_system(
        std::make_unique<RenderWalkableSpotsSystem>());
    systems.register_render_system(std::make_unique<RenderEntitySystem>());
}

}  // namespace system_manager
