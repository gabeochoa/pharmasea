
#pragma once

#include "../strings.h"
#include "ui/svg.h"
#include "ui/ui.h"

struct SVGRenderer {
    SVGNode root;
    raylib::Texture background_texture;
    std::string svg_name;

    explicit SVGRenderer(const std::string& svg_name) : svg_name(svg_name) {
        // TODO replace with actual file loading
        std::string svg = fmt::format("./resources/ui/{}.svg", svg_name);
        root = load_and_parse(svg);
        background_texture = TextureLibrary::get().get(svg_name);
    }

    void draw_background() {
        float scale = background_texture.width / 1280.f;
        raylib::DrawTextureEx(background_texture, {0, 0}, 0.f, scale, WHITE);
    }

    ui::ElementResult button(const std::string& id,
                             const TranslatableString& content) {
        auto element = SVGNode::find_matching_id(root, id);
        if (!element.has_value()) {
            log_warn("Failed to find {} in svg {}", id, svg_name);
            return ui::ElementResult{false};
        }
        Rectangle rect = element.value().get_and_scale_rect();
        // log_info("id {} @ {} ", id, rect);
        return ui::button(ui::Widget{rect}, content, false);
    }

    ui::ElementResult text(const std::string& id,
                           const TranslatableString& content) {
        auto element = SVGNode::find_matching_id(root, id);
        if (!element.has_value()) {
            log_warn("Failed to find {} in svg {}", id, svg_name);
            return ui::ElementResult{false};
        }
        Rectangle rect = element.value().get_and_scale_rect();
        // log_info("id {} @ {} ", id, rect);
        return ui::text(ui::Widget{rect}, content);
    }
};
