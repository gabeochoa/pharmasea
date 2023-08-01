
#pragma once

#include "../../external_include.h"
#include "../../strings.h"
#include "../ui.h"
#include "parsing.h"

inline LayoutBox build_layout_tree(const Node& node) {
    auto root_box = LayoutBox{
        .display_type = node.display(),
        .style = node.style,
        .node = node,
    };

    for (auto& child : node.children) {
        if (child.display() == None) continue;
        if (child.display() == Inline) {
            root_box.get_inline_container().children.push_back(
                build_layout_tree(child));
            continue;
        }
        if (child.display() == Block) {
            root_box.children.push_back(build_layout_tree(child));
        }
    }

    return root_box;
}

inline void dump(const LayoutBox& root_box, int indent = 0) {
    std::cout << std::string(indent, ' ') << root_box << std::endl;
    for (auto& child : root_box.children) {
        dump(child, indent + 4);
    }
}

inline std::optional<Node> get_body(Node root) {
    if (root.tag.empty()) return {};
    if (root.tag == "body") return root;
    for (auto child : root.children) {
        auto r = get_body(child);
        if (r.has_value()) return r;
    }
    return {};
}

inline int ui_main() {
    // Load the UI markup and CSS styles from files or strings
    Node root = load_and_parse("resources/html/simple.html");

    Node body = get_body(root).value();

    LayoutBox root_box = build_layout_tree(body);
    root_box.layout(Dimensions{.content = Rectangle{0, 0, 1280, 720}});

    log_info("_____");

    dump(root);
    dump(root_box);
    return 0;
}

inline LayoutBox load_ui(const std::string& file, raylib::Rectangle parent) {
    Node root = load_and_parse(file);
    Node body = get_body(root).value();
    LayoutBox root_box = build_layout_tree(body);
    root_box.layout(Dimensions{.content = parent});

    dump(root);
    dump(root_box);
    return root_box;
}

inline void render_ui(std::shared_ptr<ui::UIContext> ui_context,
                      const LayoutBox& root_box, raylib::Rectangle parent) {
    using namespace ui;

    Node node = root_box.node;

    if (node.tag.empty()) {
        ui_context->_draw_text(parent, text_lookup(node.content.c_str()),
                               ui::theme::Usage::Font);
        return;
    }

    switch (hashString(node.tag)) {
        case hashString("em"):
        case hashString("div"):
        case hashString("button"):
            ui_context->draw_widget_rect(root_box.dims.content,
                                         theme::Usage::Primary);
            break;
        case hashString("h1"):
        case hashString("p"):
            break;
        case hashString("html"):
        case hashString("body"):
            break;
        case hashString("head"):
        case hashString("style"):
            return;
        default:
            log_warn("trying to render {} but we dont support that tag",
                     node.tag);
            return;
    }

    for (const auto& child : root_box.children) {
        render_ui(ui_context, child, root_box.dims.content);
    }
}
