
#pragma once

#include <variant>

#include "../../external_include.h"
#include "../../strings.h"
#include "../texture_library.h"
#include "../uuid.h"
#include "elements.h"
#include "node.h"
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

inline void render_input(
    const LayoutBox& root_box, raylib::Rectangle parent,
    const std::function<void(std::string id)>& onClick,
    const std::function<elements::InputDataSource(std::string)>&
        getInputDataSource,
    const std::function<void(std::string, elements::ElementResult)>&
        processInput) {
    using namespace ui;
    Node node = root_box.node;
    auto widget = elements::Widget{root_box, node.id};

    auto input_type = node.attrs.at("type");
    if (input_type.empty()) {
        log_warn(
            "you have an input but didnt set the type... pretending its a "
            "checkbox");
        input_type = "checkbox";
    }

    auto id_attr = node.attrs.at("id");

    switch (hashString(input_type)) {
        case hashString("checkbox"):
            if (auto result = elements::checkbox(widget); result) {
                processInput(id_attr, result);
            }
            break;
        case hashString("range"):
            if (auto result = elements::slider(widget); result) {
                processInput(id_attr, result);
            }
            break;
        case hashString("dropdown"):
            if (!getInputDataSource) {
                log_warn(
                    "rendering dropdown but you didnt provide datasource "
                    "fetcher");
                return;
            }
            elements::InputDataSource data =
                getInputDataSource(node.attrs.at("id"));
            if (!std::holds_alternative<elements::DropdownData>(data)) {
                log_warn("rendering dropdown but you didnt provide options");
                return;
            }
            auto options = std::get<elements::DropdownData>(data);
            if (auto result = elements::dropdown(widget, options); result) {
                processInput(id_attr, result);
            }
            break;
    }
}

inline void render_ui(
    const LayoutBox& root_box, raylib::Rectangle parent,
    const std::function<void(std::string id)>& onClick,
    const std::function<elements::InputDataSource(std::string)>&
        getInputDataSource = {},
    const std::function<void(std::string, elements::ElementResult)>&
        processInput = {}) {
    using namespace ui;

    Node node = root_box.node;

    auto widget = elements::Widget{root_box, node.id};

    if (node.tag.empty()) {
        elements::text(widget, node.content, parent);
        return;
    }

    switch (hashString(node.tag)) {
        case hashString("button"):
            if (elements::button(widget)) {
                onClick(root_box.node.attrs.at("id"));
            }
            break;
        case hashString("div"):
            elements::div(widget);
            break;
        case hashString("input"):
            render_input(root_box, parent, onClick, getInputDataSource,
                         processInput);
            break;
        case hashString("em"):
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
        render_ui(child, root_box.dims.content, onClick, getInputDataSource,
                  processInput);
    }
}
