
#include "rendering_system.h"

#include <regex>

#include "../ah.h"
#include "../building_locations.h"
#include "../components/adds_ingredient.h"
#include "../components/ai_clean_vomit.h"
#include "../components/ai_drinking.h"
#include "../components/ai_play_jukebox.h"
#include "../components/ai_use_bathroom.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/has_fishing_game.h"
#include "../components/has_patience.h"
#include "../components/has_subtype.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_free_in_store.h"
#include "../components/is_nux_manager.h"
#include "../components/is_squirter.h"
#include "../components/is_toilet.h"
#include "../components/transform.h"
#include "../dataclass/upgrades.h"
#include "../drawing_util.h"
#include "../engine/settings.h"
#include "../engine/texture_library.h"
#include "../engine/ui/theme.h"
#include "../engine/util.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../preload.h"
#include "../vendor_include.h"
#include "raylib.h"
#include "system_manager.h"
//
#include "../engine/frustum.h"
#include "../engine/shader_library.h"

namespace system_manager {
namespace job_system {

void render_job_visual(const Entity& entity, float) {
    if (entity.is_missing<CanPathfind>()) return;
    const float box_size = TILESIZE / 10.f;
    entity.get<CanPathfind>().for_each_path_location([box_size](vec2 location) {
        DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
    });
}
}  // namespace job_system
namespace render_manager {

struct ProgressBarConfig {
    enum Type { Horizontal, Vertical } type = Horizontal;
    vec3 position = {0, 0, 0};
    vec3 scale = {1, 1, 1};
    float pct_full = 1.f;
    float y_offset = -1;
    float x_offset = -1;
    // TODO rename this to use_default_colors
    bool use_color = false;
    std::optional<Color> color_override;
    float marker_pct = -1.f;
};

void DrawFishingGame(const ProgressBarConfig& config) {
    Color primary = config.use_color
                        ? ui::get_default_progress_bar_color(config.pct_full)
                        : ui::UI_THEME.from_usage(ui::theme::Usage::Primary);
    if (config.color_override) primary = config.color_override.value();

    Color background = ui::UI_THEME.from_usage(ui::theme::Usage::Background);

    float y_offset =
        config.y_offset != -1 ? config.y_offset : 1.75f * config.scale.x;
    float x_offset = config.x_offset != -1 ? config.x_offset : 0;
    const vec3 pos = config.position;

    raylib::rlPushMatrix();
    {
        raylib::rlTranslatef(pos.x, pos.y, pos.z);

        vec3 size = {
            config.scale.x,
            config.scale.y / 3.f,
            config.scale.z / 10.f,
        };

        if (config.marker_pct > 0.f) {
            vec3 marker_pos = {
                x_offset + (config.scale.x / 4.f),
                y_offset + (config.scale.y / 5.f),  //
                -config.scale.z / 10.f,             //
            };
            int num_pieces = 20;
            int best_slice = ((int) (config.marker_pct * num_pieces));
            int current_slice = ((int) (config.pct_full * num_pieces));
            float piece_size = 1.f / num_pieces;

            const auto slice_color = [&](float pct_dist) {
                if (pct_dist < 0.05f) return GREEN;
                if (pct_dist < 0.20f) return BLUE;
                return background;
            };

            for (int i = 0; i < num_pieces; i += 1) {
                int dist_to_best = std::abs(i - best_slice);
                float pct_dist = (1.f * dist_to_best) / num_pieces;

                Color c = slice_color(pct_dist);
                if (i == current_slice) c = primary;
                const float sizey =
                    fmax(0.000001f, util::lerp(size.y, 0, pct_dist));

                DrawCubeCustom(
                    marker_pos +
                        vec3{((float) i * piece_size) - (size.x / 2.f), 0, 0},
                    piece_size, sizey, size.z / 10.f, 0, c, c);
            }
        }
    }
    raylib::rlPopMatrix();
}

void DrawProgressBar(const ProgressBarConfig& config) {
    Color primary = config.use_color
                        ? ui::get_default_progress_bar_color(config.pct_full)
                        : ui::UI_THEME.from_usage(ui::theme::Usage::Primary);
    if (config.color_override) primary = config.color_override.value();

    Color background = ui::UI_THEME.from_usage(ui::theme::Usage::Background);

    float y_offset =
        config.y_offset != -1 ? config.y_offset : 1.75f * config.scale.x;
    float x_offset = config.x_offset != -1 ? config.x_offset : 0;
    const vec3 pos = config.position;

    if (config.type == ProgressBarConfig::Horizontal) {
        raylib::rlPushMatrix();
        {
            raylib::rlTranslatef(pos.x, pos.y, pos.z);

            vec3 size = {
                config.scale.x,
                config.scale.y / 3.f,
                config.scale.z / 10.f,
            };
            const float x_size =
                fmax(0.000001f, util::lerp(0, size.x, config.pct_full));

            if (x_size > 0.f) {
                DrawCubeCustom(
                    {
                        x_offset + (x_size / 2.f) - (config.scale.x / 4.f),
                        y_offset,  //
                        0          //
                    },
                    x_size, size.y, size.z, 0, primary, primary);
            }

            if (size.x - x_size > 0.f) {
                DrawCubeCustom(
                    {
                        x_offset + (x_size / 2.f) + (config.scale.x / 4.f),
                        y_offset,  //
                        0          //
                    },
                    size.x - x_size, size.y, size.z, 0, background, background);
            }
        }
        raylib::rlPopMatrix();
        return;
    }

    raylib::rlPushMatrix();
    {
        raylib::rlTranslatef(pos.x, pos.y, pos.z);

        vec3 size = {
            config.scale.x / 3.f,
            config.scale.y,
            config.scale.z / 10.f,
        };

        const float y_size =
            fmax(0.000001f, util::lerp(0, size.y, config.pct_full));

        if (y_size > 0.f) {
            DrawCubeCustom(
                {
                    x_offset,  //
                    y_offset + (y_size / 2.f) - (config.scale.y / 4.f),
                    0  //
                },
                size.x, y_size, size.z, 0, primary, primary);
        }

        if ((size.y - y_size) > 0.f) {
            DrawCubeCustom(
                {
                    x_offset,  //
                    y_offset + (y_size / 2.f) + (config.scale.y / 4.f),
                    0  //
                },
                size.x, size.y - y_size, size.z, 0, background, background);
        }
    }
    raylib::rlPopMatrix();
}

void draw_valid_colored_box(const Transform& transform,
                            const SimpleColoredBoxRenderer& renderer,
                            bool is_highlighted) {
    Color f = renderer.face();
    // TODO Maybe should move into DrawCubeCustom
    if (afterhours::colors::is_empty(f)) {
        log_warn("Face color is empty");
        f = PINK;
    }

    Color b = renderer.base();
    if (afterhours::colors::is_empty(b)) {
        log_warn("Base color is empty");
        b = PINK;
    }

    if (is_highlighted) {
        f = afterhours::colors::get_highlighted(f);
        b = afterhours::colors::get_highlighted(b);
    }

    DrawCubeCustom(transform.raw() + transform.viz_offset(), transform.sizex(),
                   transform.sizey(), transform.sizez(), transform.facing, f,
                   b);
}

bool draw_transform_with_model(const Transform& transform,
                               const ModelRenderer& renderer, Color color) {
    if (renderer.missing()) return false;

    // If no models were loaded, force ENABLE_MODELS to false
    if (ModelLibrary::get().size() == 0) {
        ENABLE_MODELS = false;
    }

    ModelInfo& model_info = renderer.model_info();

    float rotation_angle = 180.f + transform.facing;
    vec3 position = {
        transform.pos().x + transform.viz_x() + model_info.position_offset.x,
        transform.pos().y + transform.viz_y() + model_info.position_offset.y,
        transform.pos().z + transform.viz_z() + model_info.position_offset.z,
    };

    if (ENABLE_MODELS) {
        // Apply lighting shader to model materials (Blinn-Phong/Half-Lambert).
        // NOTE: ModelRenderer::model() returns a copy; we must set the shader
        // on the library-owned model reference to persist.
        auto& model_ref = ModelLibrary::get().get(renderer.name());

        const bool lighting_enabled = Settings::get().data.enable_lighting;
        if (lighting_enabled) {
            auto& lighting_shader = ShaderLibrary::get().get("lighting");
            for (int i = 0; i < model_ref.materialCount; i++) {
                if (model_ref.materials[i].shader.id != lighting_shader.id) {
                    model_ref.materials[i].shader = lighting_shader;
                }
            }
        } else {
            raylib::Shader default_shader{};
            default_shader.id = raylib::rlGetShaderIdDefault();
            default_shader.locs = raylib::rlGetShaderLocsDefault();
            for (int i = 0; i < model_ref.materialCount; i++) {
                if (model_ref.materials[i].shader.id != default_shader.id) {
                    model_ref.materials[i].shader = default_shader;
                }
            }
        }

        DrawModelEx(model_ref, position, vec3{0.f, 1.f, 0.f},
                    model_info.rotation_angle + rotation_angle,
                    transform.size() * model_info.size_scale, color);
    } else {
        // Draw a cube as fallback when models are disabled
        vec3 size = transform.size() * model_info.size_scale * 0.8f;
        DrawCubeV(position, size,
                  RED);  // Use red color to make them more visible
    }

    return true;
}

bool draw_internal_model(const Entity& entity, Color color) {
    if (entity.is_missing<ModelRenderer>()) return false;
    if (entity.is_missing<Transform>()) return false;

    const Transform& transform = entity.get<Transform>();
    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    return draw_transform_with_model(transform, renderer, color);
}

void update_character_model_from_index(Entity& entity, float) {
    if (entity.is_missing<UsesCharacterModel>()) return;
    UsesCharacterModel& usesCharacterModel = entity.get<UsesCharacterModel>();

    if (usesCharacterModel.value_same_as_last_render()) return;

    if (entity.is_missing<ModelRenderer>()) return;

    ModelRenderer& renderer = entity.get<ModelRenderer>();

    // TODO this should be the same as all other rendere updates for players
    renderer.update_model_name(usesCharacterModel.fetch_model_name());
    usesCharacterModel.mark_change_completed();
}

bool render_simple_highlighted(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();
    draw_valid_colored_box(transform, renderer, true);
    return true;
}

bool render_simple_normal(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();
    if (entity.is_missing<SimpleColoredBoxRenderer>()) return false;
    const SimpleColoredBoxRenderer& renderer =
        entity.get<SimpleColoredBoxRenderer>();

    draw_valid_colored_box(transform, renderer, false);
    return true;
}

bool render_bounding_box(const Entity& entity, float) {
    if (entity.is_missing<Transform>()) return false;
    const Transform& transform = entity.get<Transform>();

    DrawFloatingText(transform.raw(), Preload::get().font,
                     fmt::format("{}", entity.id).c_str());

    {
        DrawBoundingBox(transform.bounds(), MAROON);
        Rectangle rect_bounds = transform.rectangular_bounds();
        DrawCubeCustom({rect_bounds.x, -1.f * (TILESIZE / 2.f), rect_bounds.y},
                       rect_bounds.width, TILESIZE / 7.f, rect_bounds.height,
                       transform.facing, MAROON, MAROON);
    }

    return true;
}

void render_debug_subtype(const Entity& entity, float) {
    const Transform& transform = entity.get<Transform>();
    std::string content;

    if (entity.has<HasSubtype>()) {
        const HasSubtype& hs = entity.get<HasSubtype>();
        content = fmt::format("{}", hs.get_type_index());
        // Convert from ID to ingredient if its an alcohol
        if (check_type(entity, EntityType::Alcohol)) {
            content = fmt::format("{}", hs.as_type<Ingredient>());
        }
        if (check_type(entity, EntityType::Fruit)) {
            Ingredient ig = ingredient::Fruits[hs.get_type_index()];
            content = fmt::format("{}", magic_enum::enum_name<Ingredient>(ig));
        }
    }

    if (entity.has<IsPnumaticPipe>()) {
        const IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();
        content = fmt::format("{} -> {}", entity.id, ipp.paired.id);
    }

    DrawFloatingText(vec::raise(transform.raw(), 1.f), Preload::get().font,
                     content.c_str());
}

void render_debug_drink_info(const Entity& entity, float) {
    if (entity.is_missing<IsDrink>()) return;
    const IsDrink& isdrink = entity.get<IsDrink>();
    const Transform& transform = entity.get<Transform>();

    float y = 0.25f;

    if (isdrink.underlying.has_value()) {
        const auto num_complete =
            fmt::format("({})", isdrink.get_num_complete());
        auto content = fmt::format(
            "{}{}",
            std::string(
                magic_enum::enum_name<Drink>(isdrink.underlying.value())),
            isdrink.get_num_complete() > 1 ? num_complete : "");
        DrawFloatingText(vec::raise(transform.raw(), y), Preload::get().font,
                         std::string(content).c_str(), 100);
    }

    y += 0.25f;
    for (size_t i = 0; i < magic_enum::enum_count<Ingredient>(); i++) {
        Ingredient ingredient = magic_enum::enum_value<Ingredient>(i);
        if (!(isdrink.has_ingredient(ingredient))) continue;
        const auto num_igs =
            fmt::format("({})", isdrink.count_of_ingredient(ingredient));
        auto content = fmt::format(
            "{} {}", magic_enum::enum_name(ingredient),
            isdrink.count_of_ingredient(ingredient) > 1 ? num_igs : "");
        DrawFloatingText(vec::raise(transform.raw(), y), Preload::get().font,
                         std::string(content).c_str(), 50);
        y += 0.25f;
    }
}

void render_debug_num_uses_left(const Entity& entity, float) {
    if (entity.is_missing<AddsIngredient>()) return;
    const AddsIngredient& addsig = entity.get<AddsIngredient>();
    int uses = addsig.uses_left();
    if (uses <= 1)  // include 0 and -1
        return;

    const Transform& transform = entity.get<Transform>();

    const auto content = fmt::format("Uses Remaining: {}", uses);
    DrawFloatingText(vec::raise(transform.raw(), 0.5f), Preload::get().font,
                     std::string(content).c_str(), 50);
}

template<typename C>
void _render_ai_target_if_exists(const Entity& entity) {
    if (entity.is_missing<C>()) return;
    const C& ai_component = entity.get<C>();

    // no active target
    if (ai_component.target.missing()) return;

    OptEntity opt_target =
        EntityHelper::getEntityForID(ai_component.target.id());

    // Didnt find the target for some reason...
    if (!opt_target.valid()) return;

    // TODO I would like for non debug mode if there would be a tooltip
    // with the customer's face or something so we know who is going there
    Entity& target = opt_target.asE();
    vec3 position = target.get<Transform>().pos();
    DrawCubeCustom(position, 1.f, 1.f, 1.f, 0, ui::color::hot_pink,
                   ui::color::hot_pink);
}

void render_debug_ai_info(const Entity& entity, float) {
    if (entity.is_missing<CanPerformJob>()) return;

    const Transform& transform = entity.get<Transform>();

    const CanPerformJob& cpj = entity.get<CanPerformJob>();

    const auto content =
        fmt::format("{}", magic_enum::enum_name<JobType>(cpj.current));
    DrawFloatingText(vec::raise(transform.raw(), 2.0f), Preload::get().font,
                     std::string(content).c_str(), 150);

    _render_ai_target_if_exists<AICleanVomit>(entity);
    _render_ai_target_if_exists<AIUseBathroom>(entity);
    _render_ai_target_if_exists<AIDrinking>(entity);
    _render_ai_target_if_exists<AIWaitInQueue>(entity);
    _render_ai_target_if_exists<AIPlayJukebox>(entity);
}

void render_debug_filter_info(const Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;
    const EntityFilter& filter = entity.get<CanHoldItem>().get_filter();
    if (filter.flags == 0) return;
    if (!filter.filter_is_set()) return;

    const Transform& transform = entity.get<Transform>();

    float y = 0.25f;
    for (size_t i = 1; i < filter.type_count(); i++) {
        EntityFilter::FilterDatumType type =
            magic_enum::enum_value<EntityFilter::FilterDatumType>(i);

        auto filter_name = magic_enum::enum_name(type);
        auto filter_value = filter.print_value_for_type(type);

        auto content = fmt::format("filter {}: {}", filter_name, filter_value);

        DrawFloatingText(vec::raise(transform.raw(), y), Preload::get().font,
                         std::string(content).c_str(), 50);
        y += 0.25f;
    }
}

bool render_debug(const Entity& entity, float dt) {
    job_system::render_job_visual(entity, dt);

    render_debug_subtype(entity, dt);
    render_debug_drink_info(entity, dt);
    render_debug_filter_info(entity, dt);
    render_debug_ai_info(entity, dt);

    // Ghost player only render during debug mode
    if (entity.has<CanBeGhostPlayer>() &&
        entity.get<CanBeGhostPlayer>().is_ghost()) {
        render_simple_normal(entity, dt);
    }
    return render_bounding_box(entity, dt);
}

bool render_model_highlighted(const Entity& entity, float) {
    if (entity.is_missing<CanBeHighlighted>()) return false;
    return draw_internal_model(entity, GRAY);
}

bool render_model_normal(const Entity& entity, float) {
    return draw_internal_model(entity, WHITE);
}

std::tuple<std::string, int, int> get_texture_name_and_position(
    const std::string& input) {
    std::regex pattern("\\[(.*?)\\]");
    std::match_results<std::string::const_iterator> what;
    if (!std::regex_search(input, what, pattern)) {
        return {"", -1, -1};
    }
    size_t position =
        what.position(0);  // return the index of the '[' character

    auto extract_string = [](const std::string& input,
                             size_t index) -> std::string {
        size_t start = index;  // index of the '[' character
        size_t end =
            input.find(']', start);  // find the index of the ']' character
        if (end != std::string::npos) {
            return input.substr(start + 1,
                                end - start - 1);  // extract the string inside
        }
        return "";  // return an empty string if no ']' character is found
    };

    auto extracted = extract_string(input, position);
    if (!TextureLibrary::get().contains(extracted)) return {"", -1, -1};
    return {extracted, position, input.find(']', position)};
}

// TODO at some point move to the other drawing utils
static void DrawFloatingTextWithIcons(const std::string& content,
                                      const vec3& position, font::Font font,
                                      int size = 96, Color color = BLACK,
                                      bool backface = true) {
    auto tex_position = get_texture_name_and_position(content);

    // No texture found, just draw normally
    if (std::get<1>(tex_position) == -1) {
        return DrawFloatingText(position, font, content.c_str(), size, color,
                                backface);
    }

    int start = std::get<1>(tex_position);
    int end = std::get<2>(tex_position);

    std::string post = content;
    post.replace(start, end - start + 1, "    ");

    DrawFloatingText(position, font, post.c_str(), size, color, backface,
                     std::get<0>(tex_position), start);
}

void render_nux(const Entity& entity, float) {
    if (entity.is_missing<IsNux>()) return;

    const IsNux& nux = entity.get<IsNux>();
    const Transform& transform = entity.get<Transform>();

    if (!nux.is_active) return;

    vec3 entity_pos = transform.pos();
    if (nux.is_attached()) {
        OptEntity attached_opt = EntityHelper::getEntityForID(nux.entityID);
        // if the attached entity is a player we have to do something else
        if (!attached_opt.has_value()) {
            attached_opt = EQ(SystemManager::get().oldAll)
                               .whereID(nux.entityID)
                               .gen_first();
        }

        if (attached_opt.has_value()) {
            entity_pos = attached_opt->get<Transform>().pos();
        }
    }

    // _render_tooltip
    {
        const float scale = 2.f;

        const auto font = Preload::get().font;
        const auto nux_position = entity_pos + vec3{-1.f, 2.f * TILESIZE, 0};

        DrawCubeCustom(nux_position + vec3{1.f, 0.5f, -0.1f}, scale * 4.f,
                       0.5f * scale, 0.10f, 0.f, WHITE, BLACK);

        DrawFloatingTextWithIcons(translation_lookup(nux.content), nux_position,
                                  font, static_cast<int>(96 * scale), BLACK,
                                  false);
    }

    // _render_ghost
    {
        if (nux.ghost != EntityType::Unknown) {
            ModelRenderer ghost(nux.ghost);

            bool has_model = draw_transform_with_model(
                transform, ghost,
                afterhours::colors::set_opacity(ui::color::green_apple, 100));

            if (!has_model) {
                DrawCubeCustom(transform.pos(), 1.f, 1.f, 1.f, 0,
                               afterhours::colors::set_opacity(
                                   ui::color::green_apple, 100),
                               afterhours::colors::set_opacity(
                                   ui::color::green_apple, 100));
            }
        }
    }
}

void render_trigger_area(const Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;

    const IsTriggerArea& ita = entity.get<IsTriggerArea>();
    const Transform& transform = entity.get<Transform>();

    vec3 pos = transform.pos();
    vec3 size = transform.size();

    vec3 text_position = {pos.x - (size.x / 2.f),     //
                          pos.y + (TILESIZE / 10.f),  //
                          pos.z - (size.z / 3.f)};

    vec3 number_position = {pos.x + (size.x / 3.f),
                            pos.y + (TILESIZE / 5.f),  //
                            pos.z + (size.z / 4.f)};
    // Top left text position
    // {pos.x - (size.x / 2.f),     //
    // pos.y + (TILESIZE / 20.f),  //
    // pos.z - (size.z / 2.f)};

    auto font = Preload::get().font;
    // TODO eventually we can detect the size to fit the text correctly
    // but thats more of an issue for translations since i can manually
    // place the english
    auto fsize_per_width = 62.f;
    auto fsize = fsize_per_width * size.x;
    // Save slot labels are intentionally more verbose (multi-line), so render
    // them a bit smaller to avoid overflowing.
    if (ita.type == IsTriggerArea::LoadSave_LoadSlot ||
        ita.type == IsTriggerArea::Planning_SaveSlot) {
        fsize *= 0.78f;
    }

    TranslatableString title = entity.get<IsTriggerArea>().title();
    log_ifx(title.empty(), LogLevel::LOG_WARN,
            "Rendering trigger area with empty text string: id{} pos{}",
            entity.id, pos);

    if (ita.should_wave()) {
        raylib::DrawTextConfig textConfig = {
            .font = font,
            .text = fmt::format("~~{}~~", translation_lookup(TranslatableString(
                                              ita.subtitle()))),
            .position = text_position,
            .fontSize = fsize,
            .fontSpacing = 4,
            .lineSpacing = 4,
            .backface = false,
            .color = WHITE,
            //
        };

        raylib::WaveTextConfig waveConfig = {.waveRange = {0, 0, 1},
                                             .waveSpeed = {0.0f, 0.0f, 0.01f},
                                             .waveOffset = {0.f, 0.f, 0.2f}};

        raylib::DrawTextWave3D(textConfig,             //
                               waveConfig,             //
                               now::current_hrc_ms(),  //
                               WHITE);

    } else {
        raylib::DrawTextConfig titleConfig = {
            .font = font,
            .text = translation_lookup(title),
            .position = text_position,
            .fontSize = fsize,
            .fontSpacing = 4,
            .lineSpacing = 4,
            .backface = false,
            .color = WHITE,
        };
        raylib::DrawText3D(titleConfig);

        auto text =
            fmt::format("{}/{}", ita.active_entrants(), ita.min_req_entrants());

        raylib::DrawTextConfig metaConfig = {
            .font = font,
            .text = text,
            .position = number_position,
            .fontSize = fsize / 2.f,
            .fontSpacing = 4,
            .lineSpacing = 4,
            .backface = false,
            .color = WHITE,
        };
        raylib::DrawText3D(metaConfig);
    }

    if (ita.progress() > 0.f) {
        // TODO switch to using a validated draw_cube
        DrawCubeCustom(
            {
                (pos.x + ((size.x * ita.progress()) / 2.f)) -
                    (size.x / 2.f),         //
                pos.y + (TILESIZE / 20.f),  //
                pos.z                       //
            },
            size.x * ita.progress(),  //
            size.y,                   //
            size.z, transform.facing, RED, RED);
    }

    render_simple_normal(entity, dt);

    const auto _render_drink_preview = [&](Drink drink) {
        const auto font = Preload::get().font;
        const auto start_position =
            transform.raw() + vec3{0, 1.0f * TILESIZE, -2.f * TILESIZE};

        raylib::DrawFloatingText(start_position, font,
                                 get_string_for_drink(drink).c_str());

        auto ingredients = get_recipe_for_drink(drink);
        int i = 0;

        bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            raylib::DrawFloatingText(
                start_position - vec3{0, 0.3f * (i + 1), 0}, font,
                // TODO translate?
                fmt::format("{}", get_string_for_ingredient(ig)).c_str());
            i++;
            return bitset_utils::ForEachFlow::NormalFlow;
        });
    };

    const auto _render_upgrade_preview = [&](const UpgradeClass& uc) {
        auto impl = make_upgrade(uc);
        // TODO add some fallback
        if (!impl) {
            // no options set yet
            return;
        }

        const auto font = Preload::get().font;
        const auto start_position =
            transform.raw() + vec3{0, 1.0f * TILESIZE, -2.f * TILESIZE};

        raylib::DrawFloatingText(start_position, font,
                                 translation_lookup(impl->name).c_str());
        int i = 0;

        raylib::DrawFloatingText(start_position - vec3{0, 0.3f * (i++ + 1), 0},
                                 font,
                                 translation_lookup(impl->flavor_text).c_str());

        raylib::DrawFloatingText(start_position - vec3{0, 0.3f * (i++ + 1), 0},
                                 font,
                                 translation_lookup(impl->description).c_str());
    };

    const auto _render_progression_option = [&](IsTriggerArea::Type type) {
        if (!PROGRESSION_BUILDING.is_inside(
                SystemManager::get().local_players[0]->get<Transform>().as2()))
            return;
        OptEntity sophie =
            EntityQuery().whereType(EntityType::Sophie).gen_first();
        const IsProgressionManager& ipm = sophie->get<IsProgressionManager>();

        bool isOption1 = type == IsTriggerArea::Progression_Option1;

        switch (ipm.upgrade_type()) {
            case UpgradeType::None:
                break;
            case UpgradeType::Upgrade: {
                _render_upgrade_preview(isOption1 ? ipm.upgradeOption1
                                                  : ipm.upgradeOption2);
            } break;
            case UpgradeType::Drink: {
                _render_drink_preview(isOption1 ? ipm.drinkOption1
                                                : ipm.drinkOption2);
            } break;
        }
    };

    switch (ita.type) {
        case IsTriggerArea::Unset:
        case IsTriggerArea::Lobby_PlayGame:
        case IsTriggerArea::Lobby_ModelTest:
        case IsTriggerArea::Store_BackToPlanning:
        case IsTriggerArea::ModelTest_BackToLobby:
        case IsTriggerArea::Store_Reroll:
        case IsTriggerArea::Lobby_LoadSave:
        case IsTriggerArea::LoadSave_BackToLobby:
        case IsTriggerArea::LoadSave_LoadSlot:
        case IsTriggerArea::LoadSave_ToggleDeleteMode:
        case IsTriggerArea::Planning_SaveSlot:
            break;
        case IsTriggerArea::Progression_Option1:
        case IsTriggerArea::Progression_Option2:
            _render_progression_option(ita.type);
            break;
    }
}

void render_texture_billboard(const Transform& transform,
                              const std::string& texture_name) {
    // TODO cant seem to find trash texture, and is crashing?

    vec3 position = transform.pos();
    raylib::Texture texture = TextureLibrary::get().get(texture_name);
    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x + (TILESIZE * 0.05f),  //
                               position.y + (TILESIZE * 2.f),    //
                               position.z},                      //
                          0.75f * TILESIZE,                      //
                          raylib::WHITE);
}

void render_billboard_at_entity(const OptEntity& opt_entity,
                                const std::string& texture_name) {
    if (!opt_entity.has_value()) return;
    return render_texture_billboard(opt_entity->get<Transform>(), texture_name);
}

void render_icon_for_marked_items(const IsFloorMarker& ifm,
                                  const std::string& texture_name) {
    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;
        render_billboard_at_entity(marked_entity, texture_name);
    }
}

void render_floor_marker(const Entity& entity, float) {
    if (entity.is_missing<IsFloorMarker>()) return;

    const Transform& transform = entity.get<Transform>();

    vec3 pos = transform.pos();
    vec3 size = transform.size();

    vec3 text_position = {pos.x - (size.x / 2.f),     //
                          pos.y + (TILESIZE / 10.f),  //
                          pos.z - (size.z / 3.f)};

    auto font = Preload::get().font;
    // TODO eventually we can detect the size to fit the text correctly
    // but thats more of an issue for translations since i can manually
    // place the english
    auto fsize = 500.f;

    const auto _get_string = [](const Entity& entity) {
        switch (entity.get<IsFloorMarker>().type) {
            case IsFloorMarker::Unset:
                return NO_TRANSLATE("");
            // TODO for not use the other one but eventually probably just
            // make it invisible
            case IsFloorMarker::Store_SpawnArea:
            case IsFloorMarker::Planning_SpawnArea:
                return TranslatableString(strings::i18n::FLOORMARKER_NEW_ITEMS);
            case IsFloorMarker::Planning_TrashArea:
                return TranslatableString(strings::i18n::FLOORMARKER_TRASH);
            case IsFloorMarker::Store_LockedArea:
                return TranslatableString(
                    strings::i18n::FLOORMARKER_STORE_LOCK);
            case IsFloorMarker::Store_PurchaseArea:
                return TranslatableString(
                    strings::i18n::FLOORMARKER_STORE_PURCHASE);
        }
        return NO_TRANSLATE("");
    };

    const TranslatableString title = _get_string(entity);

    log_ifx(title.empty(), LogLevel::LOG_WARN,
            "Rendering trigger area with empty text string: id{} pos{}",
            entity.id, pos);

    raylib::DrawTextConfig textConfig = {
        .font = font,
        .text = translation_lookup(title),
        .position = text_position,
        .fontSize = fsize,
        .fontSpacing = 4,
        .lineSpacing = 4,
        .backface = false,
        .color = WHITE,
    };

    raylib::DrawText3D(textConfig);

    // we dont need this since the caller of render_floor_marker doesnt
    // return render_simple_normal(entity, dt);

    const IsFloorMarker& ifm = entity.get<IsFloorMarker>();
    switch (ifm.type) {
        case IsFloorMarker::Unset:
        case IsFloorMarker::Planning_SpawnArea:
        case IsFloorMarker::Store_SpawnArea:
            break;
        case IsFloorMarker::Planning_TrashArea:
            render_icon_for_marked_items(ifm, "trashcan");
            break;
        case IsFloorMarker::Store_PurchaseArea:
            render_icon_for_marked_items(ifm, "dollar_sign");
            break;
        case IsFloorMarker::Store_LockedArea:
            render_icon_for_marked_items(ifm, "lock");
            break;
    }
}

void render_spawner_next_customer(const Entity& entity, float) {
    if (entity.is_missing<IsSpawner>()) return;
    const IsSpawner& iss = entity.get<IsSpawner>();
    if (iss.hit_max()) return;
    if (!iss.show_progress()) return;

    const Transform& transform = entity.get<Transform>();

    float pct = iss.get_pct();
    if (pct <= 0.01f) return;

    DrawProgressBar(ProgressBarConfig{
        .type = ProgressBarConfig::Vertical,
        .position = transform.pos() + vec3{0, 0.5f, 0},
        .scale = {0.75f, 0.75f, 0.75f},
        .pct_full = pct,
        .y_offset = 0.f * TILESIZE,
        .x_offset = -0.45f * TILESIZE,
        .use_color = true,
    });
}

void render_toilet_floor_timer(const Entity& entity, float) {
    if (entity.is_missing<AIUseBathroom>()) return;
    const Transform& transform = entity.get<Transform>();
    const AIUseBathroom& aiusebathroom = entity.get<AIUseBathroom>();

    const AITakesTime floor_timer = aiusebathroom.floor_timer;

    // no timer set yet
    if (!floor_timer.initialized) return;
    if (floor_timer.totalTime == -1) return;
    if (floor_timer.timeRemaining == -1) return;

    float pct = floor_timer.timeRemaining / floor_timer.totalTime;
    if (pct == 1.f) return;

    DrawProgressBar(ProgressBarConfig{
        .type = ProgressBarConfig::Vertical,
        .position = transform.pos(),
        .scale = {0.75f, 0.75f, 0.75f},
        .pct_full = pct,
        .y_offset = 2.f * TILESIZE,
        .x_offset = -0.45f * TILESIZE,
        .use_color = true,
    });
}

void render_patience(const Entity& entity, float) {
    if (entity.is_missing<HasPatience>()) return;
    const Transform& transform = entity.get<Transform>();
    const HasPatience& hp = entity.get<HasPatience>();
    if (hp.pct() >= 1.f) return;

    DrawProgressBar(ProgressBarConfig{
        .type = ProgressBarConfig::Vertical,
        .position = transform.pos(),
        .scale = {0.75f, 0.75f, 0.75f},
        .pct_full = hp.pct(),
        .y_offset = 2.f * TILESIZE,
        .x_offset = -0.45f * TILESIZE,
        .use_color = true,
    });
}

void render_ai_info(const Entity& entity, float) {
    if (entity.is_missing<CanPerformJob>()) return;
    const Transform& transform = entity.get<Transform>();
    const CanPerformJob& cpj = entity.get<CanPerformJob>();

    const vec3 position = transform.pos();
    const vec3 icon_position = vec3{position.x + (TILESIZE * 0.05f),  //
                                    position.y + (TILESIZE * 2.f),    //
                                    position.z};
    switch (cpj.current) {
        case Bathroom: {
            GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
            // TODO reuse the toilet upgrade one for now
            raylib::Texture texture = TextureLibrary::get().get("gotta_go");
            raylib::DrawBillboard(cam.camera, texture,
                                  // move it a bit so that it doesnt overlap
                                  icon_position + vec3{0.f, 1.f, 0},
                                  0.75f * TILESIZE, raylib::WHITE);
        } break;
        case Paying: {
            GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
            // TODO reuse the store dollar sign one for now
            raylib::Texture texture = TextureLibrary::get().get("dollar_sign");
            raylib::DrawBillboard(cam.camera, texture,
                                  // move it a bit so that it doesnt overlap
                                  icon_position + vec3{0.f, 1.f, 0},
                                  0.75f * TILESIZE, raylib::WHITE);
        } break;
        case NoJob:
        case Wait:
        case WaitInQueue:
        case Drinking:
        case Mopping:
        case PlayJukebox:
        case Wandering:
        case EnterStore:
        case WaitInQueueForPickup:
        case Leaving:
        case MAX_JOB_TYPE:
            break;
    }
}

void render_speech_bubble(const Entity& entity, float) {
    if (entity.is_missing<HasSpeechBubble>()) return;
    if (entity.get<HasSpeechBubble>().disabled()) return;

    // Right now this is the only thing we can put in a bubble
    if (entity.is_missing<CanOrderDrink>()) return;

    const Transform& transform = entity.get<Transform>();
    const vec3 position = transform.pos();

    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    const vec3 icon_position = vec3{position.x + (TILESIZE * 0.05f),  //
                                    position.y + (TILESIZE * 2.f),    //
                                    position.z};

    // TODO add way to turn on / off these icons
    if (true) {
        GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
        raylib::Texture texture = TextureLibrary::get().get(cod.icon_name());
        raylib::DrawBillboard(cam.camera, texture,
                              // move it a bit so that it doesnt overlap
                              icon_position + vec3{0.f, 1.f, 0},
                              0.75f * TILESIZE, raylib::WHITE);
    }

    // Render 3D models or fallback cubes
    const auto model_name = get_model_name_for_drink(cod.get_order());
    const ModelInfo& model_info = ModelInfoLibrary::get().get(model_name);
    vec3 model_position = icon_position + model_info.position_offset;
    vec3 model_size = transform.size() * model_info.size_scale;
    float rotation_angle = 180.f + transform.facing;

    if (ENABLE_MODELS) {
        const auto model = ModelLibrary::get().get(model_name);
        raylib::DrawModelEx(model, model_position, vec3{0, 1, 0},
                            model_info.rotation_angle + rotation_angle,
                            model_size, WHITE /*base_color*/);
    } else {
        // Draw a cube as fallback when models are disabled
        DrawCubeV(model_position, model_size, WHITE);
    }
}

void render_waiting_queue(const Entity& entity, float) {
    if (entity.is_missing<HasWaitingQueue>()) return;

    // TODO its crashing again, this time it looks like its an issue with
    // EntityHelper::isWalkable where fetching a vec2 from a map is causing a
    // segfault
    return;

    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();

    const Transform& transform = entity.get<Transform>();
    vec3 size = transform.size();

    for (size_t i = 0; i < hwq.max_queue_size; i++) {
        vec2 pos2 = transform.tile_infront((int) i + 1);
        vec3 pos = vec::to3(pos2);
        bool walkable = EntityHelper::isWalkable(pos2);
        DrawCubeCustom({pos.x, pos.y - (TILESIZE * 0.5f), pos.z}, size.x,
                       size.y / 10.f, size.z, transform.facing,
                       walkable ? ui::color::transleucent_green
                                : ui::color::transleucent_red,
                       walkable ? ui::color::transleucent_green
                                : ui::color::transleucent_red);
    }
}

void render_price(const Entity& entity, float) {
    int price = get_price_for_entity_type(get_entity_type(entity));
    if (price == -1) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    bool someone_close =
        SystemManager::get().is_some_player_near(transform.as2(), 2.f);
    if (!someone_close) return;

    vec3 location = transform.raw() + vec3{0, 0.5f * TILESIZE, 0.5f * TILESIZE};

    bool is_free = entity.has<IsFreeInStore>();
    if (is_free) price = 0;

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(location, Preload::get().font,
                             fmt::format("${}", price).c_str(), 200,
                             is_free ? GREEN : BLUE);
}

void render_machine_name(const Entity& entity, int font_size = 200) {
    if (entity.is_missing<IsSolid>()) return;
    if (entity.is_missing<Transform>()) return;
    // not really a point to show "wall" on every wall
    // so using this as a way to ignore those
    if (entity.is_missing<CanBeHeld>()) return;

    const Transform& transform = entity.get<Transform>();

    bool someone_close =
        SystemManager::get().is_some_player_near(transform.as2(), 2.f);
    if (!someone_close) return;

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(
        transform.raw() + vec3{0.3f, 0.2f, 0.5f}, Preload::get().font,
        std::string(str(get_entity_type(entity))).c_str(), font_size);
}

void render_debug_fruit_juice(const Entity& entity, float) {
    if (!check_type(entity, EntityType::FruitJuice)) return;
    if (entity.is_missing<Transform>()) return;

    const Transform& transform = entity.get<Transform>();

    if (entity.is_missing<ModelRenderer>()) {
        DrawFloatingText(vec::raise(transform.raw(), 1.f), Preload::get().font,
                         "Missing model renderer");
        return;
    }

    const ModelRenderer& renderer = entity.get<ModelRenderer>();
    if (renderer.missing()) {
        DrawFloatingText(
            vec::raise(transform.raw(), 1.f), Preload::get().font,
            fmt::format("renderer.missing for {}", renderer.name()).c_str());
        return;
    }
    ModelInfo& model_info = renderer.model_info();

    const auto content = fmt::format("{}", model_info.model_name);

    DrawFloatingText(vec::raise(transform.raw(), 1.f), Preload::get().font,
                     content.c_str());
}

void render_smelly_toilet(const Entity& entity, float) {
    if (entity.is_missing<IsToilet>()) return;

    const IsToilet& istoilet = entity.get<IsToilet>();

    raylib::Texture texture;
    switch (istoilet.state) {
        case IsToilet::Available: {
            float pct_filled = 1.f - istoilet.pct_empty();
            if (pct_filled > 0.5f) {
                texture = TextureLibrary::get().get("toilet_half_filled");
            } else {
                texture = TextureLibrary::get().get("unoccupied");
            }
        } break;
        case IsToilet::InUse:
            texture = TextureLibrary::get().get("occupied");
            break;
        case IsToilet::State::NeedsCleaning:
            texture = TextureLibrary::get().get("poo");
            break;
    }

    const Transform& transform = entity.get<Transform>();
    vec3 position = transform.pos();

    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    raylib::DrawBillboard(cam.camera, texture,
                          vec3{position.x + (TILESIZE * 0.05f),  //
                               position.y + (TILESIZE * 2.f),    //
                               position.z},                      //
                          0.75f * TILESIZE,                      //
                          raylib::WHITE);
}

// TODO theres two functions called render normal, maybe we should address
// this
void render_normal(const Entity& entity, float dt) {
    //  TODO for now while we do dev work render it
    render_debug_subtype(entity, dt);
    render_debug_drink_info(entity, dt);
    render_debug_filter_info(entity, dt);
    render_debug_fruit_juice(entity, dt);
    render_debug_num_uses_left(entity, dt);

    render_ai_info(entity, dt);

    render_waiting_queue(entity, dt);
    render_smelly_toilet(entity, dt);

    // Ghost player cant render during normal mode
    if (entity.has<CanBeGhostPlayer>() &&
        entity.get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (check_type(entity, EntityType::Face)) {
        auto cam = GLOBALS.get_ptr<GameCam>(strings::globals::GAME_CAM);
        raylib::DrawBillboard(
            cam->camera, TextureLibrary::get().get(strings::textures::FACE),
            entity.get<Transform>().pos(), TILESIZE, WHITE);

        return;
    }

    if (entity.has<IsNux>()) {
        render_nux(entity, dt);
        return;
    }

    if (entity.has<IsTriggerArea>()) {
        render_trigger_area(entity, dt);
        return;
    }

    if (entity.has<IsFloorMarker>()) {
        render_floor_marker(entity, dt);
    }

    if (entity.has<IsStoreSpawned>() &&
        STORE_BUILDING.is_inside(entity.get<Transform>().as2())) {
        render_machine_name(entity, 200);
        render_price(entity, dt);
    }
    //  Showing machine name because playtesters
    //  didnt find it clear enough that they were still in
    //  planning state.
    //
    //  They kept trying to grab the cups and start making drinks
    else if (SystemManager::get().is_bar_closed()) {
        render_machine_name(entity, 100);
    }
    // ^ adding an "else" because otherwise itll show two names when in the
    // store

    if (entity.has<CanBeHighlighted>() &&
        entity.get<CanBeHighlighted>().is_highlighted()) {
        bool used = render_model_highlighted(entity, dt);
        if (!used) {
            render_simple_highlighted(entity, dt);
        }
        return;
    }

    if (check_type(entity, EntityType::Door)) {
        if (entity.is_missing<IsSolid>()) return;
    }

    render_speech_bubble(entity, dt);
    render_patience(entity, dt);
    render_toilet_floor_timer(entity, dt);
    render_spawner_next_customer(entity, dt);

    bool used = render_model_normal(entity, dt);
    if (!used) {
        render_simple_normal(entity, dt);
    }
}

void render_floating_name(const Entity& entity, float) {
    if (entity.is_missing<HasName>()) return;
    const HasName& hasName = entity.get<HasName>();

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    // log_warn("drawing floating name {} for {} @ {}", hasName.name(),
    // entity.id, transform.pos());

    // TODO rotate the name with the camera?
    raylib::DrawFloatingText(
        transform.raw() + vec3{0, 0.5f * TILESIZE, 0.2f * TILESIZE},
        Preload::get().font, hasName.name().c_str());
}

void render_progress_bar(const Entity& entity, float) {
    if (entity.is_missing<HasWork>()) return;
    const HasWork& hasWork = entity.get<HasWork>();
    if (hasWork.dont_show_progress_bar()) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    bool someone_close =
        SystemManager::get().is_some_player_near(transform.as2());
    if (!someone_close) return;

    DrawProgressBar(ProgressBarConfig{
        .position = transform.pos(),
        .pct_full = hasWork.scale_length(1.f),
    });
}

void render_squirt_progress_bar(const Entity& entity, float) {
    if (entity.is_missing<IsSquirter>()) return;
    const IsSquirter& is_sq = entity.get<IsSquirter>();

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();

    if (is_sq.pct() >= 0.9f) return;

    DrawProgressBar(ProgressBarConfig{.position = transform.pos(),
                                      .pct_full = is_sq.pct()});
}

void render_fishing_game(const Entity& entity, float) {
    if (entity.is_missing<HasFishingGame>()) return;
    const HasFishingGame& fishing = entity.get<HasFishingGame>();
    if (!fishing.show_progress_bar()) return;

    if (entity.is_missing<Transform>()) return;
    const Transform& transform = entity.get<Transform>();
    vec3 position = transform.pos();

    if (!fishing.has_score()) {
        DrawFishingGame(ProgressBarConfig{
            .position = position,
            .pct_full = fishing.pct(),
            .color_override =
                fishing.has_score() ? WHITE : std::optional<Color>(),
            .marker_pct = fishing.best(),
        });
        return;
    }

    float spacing = (TILESIZE * 0.40f);
    float x_offset = 0;

    // TODO read icon from score sheet
    //  ^ idk what i meant by this
    raylib::Texture texture = TextureLibrary::get().get("star_filled");
    GameCam cam = GLOBALS.get<GameCam>(strings::globals::GAME_CAM);
    for (int i = 0; i < fishing.num_stars(); i++) {
        raylib::DrawBillboard(cam.camera, texture,
                              vec3{position.x + x_offset,          //
                                   position.y + (TILESIZE * 2.f),  //
                                   position.z},                    //
                              0.3f * TILESIZE,                     //
                              raylib::WHITE);
        x_offset += spacing;
    }
}

void render_walkable_spots(float) {
    // TODO For some reason this also triggers the walkable.contains
    // segfault
    return;

    if (!GLOBALS.get<bool>("debug_ui_enabled")) return;

    for (int i = -25; i < 25; i++) {
        for (int j = -25; j < 25; j++) {
            vec2 pos2 = {(float) i, (float) j};
            bool walkable = EntityHelper::isWalkable(pos2);
            if (walkable) continue;

            DrawCubeCustom(vec::to3(pos2), TILESIZE, TILESIZE + TILESIZE / 10.f,
                           TILESIZE, 0,
                           walkable ? ui::color::transleucent_green
                                    : ui::color::transleucent_red,
                           walkable ? ui::color::transleucent_green
                                    : ui::color::transleucent_red);
        }
    }
}

void render_held_furniture_preview(const Entity& entity, float) {
    if (entity.is_missing<CanHoldFurniture>()) return;
    const CanHoldFurniture& chf = entity.get<CanHoldFurniture>();
    if (chf.empty()) return;
    const Transform& transform = entity.get<Transform>();

    vec3 drop_location = entity.get<Transform>().drop_location();

    // TODO :DUPE: this logic is also in input process manager,
    // they should match so we dont have weirdness with ui not matching
    // the actual logic
    bool walkable =                                        //
        EntityHelper::isWalkable(vec::to2(drop_location))  //
        || (drop_location == chf.picked_up_at());

    if (walkable) {
        EntityID furn_id = chf.furniture_id();
        OptEntity hf = EntityHelper::getEntityForID(furn_id);
        if (hf->has<IsStoreSpawned>()) {
            if (!STORE_BUILDING.is_inside({drop_location.x, drop_location.z})) {
                // TODO add a message or something to show you cant drop it
                walkable = false;
            }
        }
    }

    // since the preview is just a box we can just use 0 for angle
    // but if we need in the future, then we can fetch the entity with:
    //
    // OptEntity hf = EntityHelper::getEntityForID(chf.furniture_id());
    // const Transform& furniture_transform = hf->get<Transform>();

    vec3 size = (transform.size() * 1.2f);
    DrawCubeCustom(drop_location, size.x, size.y, size.z, 0,
                   walkable ? GREEN : RED, walkable ? GREEN : RED);
}

}  // namespace render_manager
}  // namespace system_manager

#define LOG_RENDER_ENT_COUNT 0

#if LOG_RENDER_ENT_COUNT
static size_t num_ents_drawn = 0;
#endif

static Frustum frustum;

namespace system_manager {

namespace render_manager {

void on_frame_start() {
#if LOG_RENDER_ENT_COUNT
    log_warn("num entities drawn: {}", num_ents_drawn);
    num_ents_drawn = 0;
#endif
    frustum.fetch_data();
}

bool should_cull(const Entity& entity) {
    auto bounds = entity.get<Transform>().expanded_bounds({0, 0, 0});
    return !frustum.AABBoxIn(bounds.min, bounds.max);
}

void render(const Entity& entity, float dt, bool is_debug) {
    if (is_debug) render_debug(entity, dt);

    if (should_cull(entity)) return;

#if LOG_RENDER_ENT_COUNT
    // rough approx :)
    num_ents_drawn++;
#endif

    render_normal(entity, dt);
    render_held_furniture_preview(entity, dt);
    render_floating_name(entity, dt);
    render_progress_bar(entity, dt);
    render_fishing_game(entity, dt);
    render_squirt_progress_bar(entity, dt);
}

}  // namespace render_manager

}  // namespace system_manager

namespace system_manager {

struct OnFrameStartSystem : public afterhours::System<> {
    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) override {
        render_manager::on_frame_start();
    }
};

struct RenderWalkableSpotsSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        return GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    }

    virtual void once(const float) override {
        render_manager::render_walkable_spots(0);
    }
};

struct RenderEntitySystem : public afterhours::System<Transform> {
    mutable bool debug_mode_on = false;

    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) const override {
        debug_mode_on = GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    }

    virtual void for_each_with(const Entity& entity, const Transform&,
                               float dt) const override {
        render_manager::render(entity, dt, debug_mode_on);
    }
};

void register_render_systems(afterhours::SystemManager& systems) {
    systems.register_render_system(std::make_unique<OnFrameStartSystem>());
    systems.register_render_system(
        std::make_unique<RenderWalkableSpotsSystem>());
    systems.register_render_system(std::make_unique<RenderEntitySystem>());
}

}  // namespace system_manager
