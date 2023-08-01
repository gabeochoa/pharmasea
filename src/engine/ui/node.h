#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../external_include.h"
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

struct Value {
    enum Units { Pixels, Percentage } unit = Pixels;
    float data = 0.f;

    float to_f(float parent) const {
        switch (unit) {
            case Value::Pixels:
                return data;
            case Value::Percentage:
                return (data / 100.f) * parent;
        }
        return data;
    }
};

std::ostream& operator<<(std::ostream& os, const Value& value) {
    os << fmt::format("{} {}", value.data,
                      magic_enum::enum_name<Value::Units>(value.unit));
    return os;
}

typedef std::map<std::string, Value> ValueMap;
struct Style {
    ValueMap values;
    DisplayType display_type = Inline;

    float lookup_f(const std::string& field, const Value& def,
                   float parent_val) const {
        auto it = values.find(field);
        if (it != values.end()) {
            return it->second.to_f(parent_val);
        }
        return def.to_f(parent_val);
    }

    // Variadic function to look up multiple fields with a fallback and a
    // default value
    template<typename... Args>
    float lookup_f(const std::string& field, const std::string& fallback,
                   const Args&... args, const Value& def,
                   float parent_val) const {
        auto it = values.find(field);
        if (it != values.end()) {
            return it->second.to_f(parent_val);
        }
        // Recursively call the function with the fallback and remaining fields
        return lookup_f(fallback, args..., def, parent_val);
    }
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
    for (auto c : style.values) {
        os << fmt::format(" {}:{}, ", c.first, c.second);
    }
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
        auto parent_width = parent_block.content.width;
        auto a_uto =
            Value{.data = parent_block.content.width, .unit = Value::Pixels};
        auto zero = Value{.data = 0, .unit = Value::Pixels};

        float width = style.lookup_f("width", a_uto, parent_width);

        auto margin_left =
            style.lookup_f("margin-left", "margin", zero, parent_width);
        auto margin_right =
            style.lookup_f("margin-right", "margin", zero, parent_width);

        auto padding_left =
            style.lookup_f("padding-left", "padding", zero, parent_width);
        auto padding_right =
            style.lookup_f("padding-right", "padding", zero, parent_width);

        auto border_left =
            style.lookup_f("border-left", "border", zero, parent_width);
        auto border_right =
            style.lookup_f("border-right", "border", zero, parent_width);

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

        // if (width != auto_width && total > parent_block.content.width) {
        // if margin_left == auto: left = 0
        // if margin_right == auto: right = 0
        // }

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
        auto zero = Value{.data = 0, .unit = Value::Pixels};
        float parent_height = parent_block.content.height;

        auto margin_top =
            style.lookup_f("margin-top", "margin", zero, parent_height);
        auto margin_bottom =
            style.lookup_f("margin-bottom", "margin", zero, parent_height);

        auto padding_top =
            style.lookup_f("padding-top", "padding", zero, parent_height);
        auto padding_bottom =
            style.lookup_f("padding-bottom", "padding", zero, parent_height);

        auto border_top =
            style.lookup_f("border-top", "border", zero, parent_height);
        auto border_bottom =
            style.lookup_f("border-bottom", "border", zero, parent_height);

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
            style.values["height"] = Value{.data = 200, .unit = Value::Pixels};
        }

        if (style.values.contains("height")) {
            dims.content.height = style.values.at("height").to_f(200);
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
