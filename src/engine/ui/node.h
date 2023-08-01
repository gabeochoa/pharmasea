#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../log.h"
#include "raylib.h"

struct Node;

typedef std::pair<std::string, std::string> Attr;
typedef std::unordered_map<std::string, std::string> Attrs;
typedef std::vector<Node> Nodes;

inline raylib::Rectangle expand(const raylib::Rectangle& a, const vec4& b) {
    return (Rectangle){a.x - b.x,            //
                       a.y - b.y,            //
                       a.width + b.x + b.z,  //
                       a.height + b.y + b.w};
}

struct Dimensions {
    raylib::Rectangle content = raylib::Rectangle{0};
    vec4 border = vec4{0};
    vec4 margin = vec4{0};
    vec4 padding = vec4{0};

    raylib::Rectangle margin_box() { return expand(content, margin); }

    raylib::Rectangle border_box() { return expand(content, border); }

    raylib::Rectangle padding_box() { return expand(content, padding); }
};

std::ostream& operator<<(std::ostream& os, const Dimensions& dims) {
    os << "Dimensions:";
    os << " Content " << dims.content;
    if (dims.border != vec4{0, 0, 0, 0}) os << " border " << dims.border;
    if (dims.margin != vec4{0, 0, 0, 0}) os << " margin " << dims.margin;
    if (dims.padding != vec4{0, 0, 0, 0}) os << " padding " << dims.padding;
    return os;
}

enum DisplayType {
    Inline,
    Block,
    None,
    //
    Anonymous,
};

struct Decl {
    std::string field;
    std::string value;
};
typedef std::vector<Decl> Decls;

struct Rule {
    Decls decls;
};

struct Style {
    float width = -1.f;
    float height = -1.f;
    DisplayType display_type = Inline;

    // eventually (name, backup, backup2,..., zero)
    float lookup_f() const { return 0.f; }
};

// TODO eventually support selectors
typedef std::map<std::string, Rule> RuleMap;

struct Node {
    std::string content;
    std::string tag;
    Attrs attrs;
    Nodes children;
    Node(){};

    Style style;

    explicit Node(const std::string& content) : content(content) {}
    Node(const std::string& tag, const Attrs& attrs, const Nodes& children)
        : tag(tag), attrs(attrs), children(children) {}

    [[nodiscard]] DisplayType display() const { return style.display_type; }
};

std::ostream& operator<<(std::ostream& os, const Style& style) {
    os << fmt::format("width: {}, height: {}", style.width, style.height);
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::optional<Style>& style) {
    if (style.has_value()) os << style.value();
    return os;
}

std::ostream& operator<<(std::ostream& os, const Node& node) {
    if (node.tag.empty()) {
        os << node.content;
        return os;
    }

    os << fmt::format("<{} {} {} style=\"{}\">", node.tag, node.attrs.size(),
                      node.children.size(), node.style);
    return os;
}

namespace dom {
inline Node text(const std::string& content) { return Node(content); }
inline Node elem(const std::string& tag, const Attrs& attrs,
                 const Nodes& children) {
    return Node(tag, attrs, children);
}

}  // namespace dom

struct LayoutBox;
typedef std::vector<LayoutBox> Layouts;
struct LayoutBox {
    Dimensions dims;
    DisplayType display_type;

    Layouts children;
    Node node;
    Style style;

    void add_anon_child() {
        children.push_back(LayoutBox{
            .display_type = Anonymous,
        });
    }

    LayoutBox& get_inline_container() {
        switch (this->display_type) {
            case Inline:
                return *this;
            case Block:
                if (children.empty()) {
                    add_anon_child();
                } else if (children.back().display_type != Anonymous) {
                    add_anon_child();
                }
                return children.back();
                //
            case None:
            case Anonymous:
                break;
        }
        return *this;
    }

    void calculate_block_width(const Dimensions& parent_block) {
        auto auto_width = parent_block.content.width;

        auto width = auto_width;
        if (style.width != -1) {
            width = style.width;
        }
        auto margin_left = style.lookup_f();
        auto margin_right = style.lookup_f();
        auto border_left = style.lookup_f();
        auto border_right = style.lookup_f();
        auto padding_left = style.lookup_f();
        auto padding_right = style.lookup_f();

        float total = (      //
            margin_left +    //
            margin_right +   //
            border_left +    //
            border_right +   //
            padding_left +   //
            padding_right +  //
            width
            //
        );

        if (width != auto_width && total > parent_block.content.width) {
            // if margin_left == auto: left = 0
            // if margin_right == auto: right = 0
        }

        // TODO handle overflow underflow
        // auto underflow = parent_block.content.width - total;

        dims.content.width = width;
        dims.margin.x = margin_left;
        dims.margin.y = margin_right;
        dims.border.x = border_left;
        dims.border.y = border_right;
        dims.padding.x = padding_left;
        dims.padding.y = padding_right;
    }

    void calculate_block_position(const Dimensions& parent_block) {
        auto margin_top = style.lookup_f();
        auto margin_bottom = style.lookup_f();
        auto border_top = style.lookup_f();
        auto border_bottom = style.lookup_f();
        auto padding_top = style.lookup_f();
        auto padding_bottom = style.lookup_f();

        dims.margin.z = margin_top;
        dims.margin.w = margin_bottom;
        dims.border.z = border_top;
        dims.border.w = border_bottom;
        dims.padding.z = padding_top;
        dims.padding.w = padding_bottom;

        dims.content.x = parent_block.content.x + dims.margin.x +
                         dims.border.x + dims.padding.x;

        dims.content.y = parent_block.content.height + parent_block.content.y +
                         dims.margin.z + dims.border.z + dims.padding.z;

        // log_info("{} {} {} {} {} {} {}", node.tag, dims.content.y,
        // parent_block.content.height, parent_block.content.y,
        // dims.margin.z, dims.border.z, dims.padding.z);
    }

    void layout_block_children() {
        for (auto& child : children) {
            child.layout(dims);
            dims.content.height += child.dims.margin_box().height;
        }
    }

    void calculate_block_height() {
        if (node.tag.empty()) {
            // TODO figure out the font size
            style.height = 200;
        }

        if (style.height != -1) {
            dims.content.height = style.height;
        }
    }

    void layout_block(const Dimensions& parent_block) {
        calculate_block_width(parent_block);
        calculate_block_position(parent_block);
        layout_block_children();
        calculate_block_height();
    }

    void layout(const Dimensions& parent_block) {
        switch (display_type) {
            case Inline:
            case Anonymous:
            case None:
                //

            case Block:
                layout_block(parent_block);
                break;
        }
    }
};

std::ostream& operator<<(std::ostream& os, const LayoutBox& box) {
    os << fmt::format("{}: {}, {}", box.node.tag, box.display_type, box.dims);
    return os;
}
