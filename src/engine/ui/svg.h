
#pragma once

#include <float.h>

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../../vendor_include.h"
#include "../assert.h"
#include "../constexpr_containers.h"
#include "../log.h"
#include "raylib.h"

using Attr = std::pair<std::string, std::string>;
using Attrs = std::map<std::string, std::string>;

struct SVGNode {
    std::string tag;
    Attrs attr;
    std::vector<SVGNode> children;

    Rectangle get_and_scale_rect() const {
        const Rectangle rect = get_rect();
        return Rectangle{
            rect.x * WIN_WF(),
            rect.y * WIN_HF(),
            rect.width * WIN_WF(),
            rect.height * WIN_HF(),
        };
    }

    Rectangle get_children_rect() const {
        float min_x = FLT_MAX;
        float min_y = FLT_MAX;
        float max_x = 0.f;
        float max_y = 0.f;

        for (auto& child : children) {
            Rectangle child_rect = child.get_rect();
            if (child_rect.width == 0 || child_rect.height == 0) continue;

            min_x = std::min(min_x, child_rect.x);
            min_y = std::min(min_y, child_rect.y);
            max_x = std::max(max_x, child_rect.x + child_rect.width);
            max_y = std::max(max_y, child_rect.y + child_rect.height);
        }
        Rectangle r{min_x, min_y, max_x - min_x, max_y - min_y};
        // log_info("{} {} {}", tag, attr.at("id"), r);
        return r;
    }

    Rectangle get_rect() const {
        // Groups dont have width and height so try to max the children?
        // TODO are there other items with width and height?
        if (tag != "rect") {
            if (children.empty()) {
                return Rectangle{0, 0, 0, 0};
            }
            return get_children_rect();
        }
        std::string x = attr.contains("x") ? attr.at("x") : "0";
        std::string y = attr.contains("y") ? attr.at("y") : "0";
        std::string width = attr.contains("width") ? attr.at("width") : "0";
        std::string height = attr.contains("height") ? attr.at("height") : "0";
        Rectangle r{
            std::stof(x) / 1280.f,
            std::stof(y) / 720.f,
            std::stof(width) / 1280.f,
            std::stof(height) / 720.f,
        };
        // log_info("{} {}", attr.at("id"), r);
        return r;
    }

    std::string name() const {
        return attr.contains("id") ? attr.at("id") : "No id";
    }

    std::string print(int t = 0) const {
        std::stringstream ss;
        std::string tabs(t, '\t');

        ss << tabs << "Tag: " << tag << "\n";
        ss << tabs << "id: " << name() << "\n";
        ss << tabs << "rect: " << get_rect() << "\n";
        for (const SVGNode& child : children) {
            ss << child.print(t + 1);
        }
        return ss.str();
    }

    [[nodiscard]] bool has_matching_attr(const std::string& key,
                                         const std::string& value) const {
        return attr.contains(key) && attr.at(key) == value;
    }

    static std::optional<SVGNode> find_matching_id(SVGNode root,
                                                   const std::string& id) {
        if (root.has_matching_attr("id", id)) return root;
        if (!root.children.empty()) {
            for (auto& child : root.children) {
                std::optional<SVGNode> node = find_matching_id(child, id);
                if (node.has_value()) {
                    return node;
                }
            }
        }
        return {};
    }

    static std::optional<SVGNode> find_matching_tag(SVGNode root,
                                                    const std::string& tag) {
        if (root.tag == tag) return root;
        if (!root.children.empty()) {
            for (auto& child : root.children) {
                std::optional<SVGNode> node = find_matching_tag(child, tag);
                if (node.has_value()) {
                    return node;
                }
            }
        }
        return {};
    }
};

inline std::ostream& operator<<(std::ostream& os, const SVGNode& node) {
    os << node.print();
    return os;
}

using SVGNodes = std::vector<SVGNode>;

struct Parser {
    size_t pos;
    std::string input;

    explicit Parser(const std::string& source) : input(source) { pos = 0; }

    void validate(unsigned char c, unsigned char m, const std::string& msg) {
        VALIDATE(c == m,
                 fmt::format("{} != {} => {}", (char) c, (char) m, msg));
    }

    unsigned char next_char() const { return input[pos]; }

    bool starts_with(const std::string& pre) const {
        return input.substr(pos).find(pre) == 0;
    }

    bool is_eof() const { return pos >= input.size(); }

    unsigned char consume() {
        // std::cout << "consumed: " << input[pos] << std::endl;
        return input[pos++];
    }

    std::string consume_while(const std::function<bool(unsigned char)>& pred) {
        std::string out;
        while (!is_eof() && pred(next_char())) {
            out.push_back(consume());
        }
        return out;
    }

    void consume_whitespace() {
        consume_while([](unsigned char c) { return std::isspace(c); });
    }

    bool is_self_closing_tag(const std::string& tag) {
        std::array<std::string, 4> self_closing_tags{"rect", "path", "use",
                                                     "image"};
        return array_contains(self_closing_tags, tag);
    }

    std::string parse_tag_name() {
        return consume_while([](unsigned char c) {
            return std::isalnum(c) ||
                   // xmlns:xlink=....
                   c == ':' ||
                   // fill-rule="evenodd"
                   c == '-';
        });
    }

    Attr parse_attr() {
        const auto _parse_attr_value = [this]() {
            auto open_quote = consume();
            VALIDATE(
                open_quote == '"' || open_quote == '\'',
                fmt::format("open_quote{} {}", open_quote, "valid open quote"));
            auto value = consume_while(
                [open_quote](unsigned char c) { return c != open_quote; });
            validate(consume(), open_quote,
                     "open quote should match local quote");
            return value;
        };

        auto name = parse_tag_name();
        validate(consume(), '=', "should match equal");
        auto value = _parse_attr_value();
        return {name, value};
    }

    Attrs parse_attrs() {
        Attrs attrs;
        // this could be while(;;) but i dont trust it
        int i = 0;
        while (i++ < 20 /* max attrs */) {
            consume_whitespace();
            if (next_char() == '/') break;
            if (next_char() == '>') break;
            auto p = parse_attr();
            attrs.insert(p);
        }
        return attrs;
    }

    SVGNode parse_element() {
        validate(consume(), '<', "parsing an tag open");
        auto tag_name = parse_tag_name();
        auto attrs = parse_attrs();

        SVGNodes children;

        if (next_char() == '/') {
            auto err_msg = fmt::format(
                "Found self closing but tag wasnt self closing: {}", tag_name);
            VALIDATE(is_self_closing_tag(tag_name), err_msg);
            validate(consume(), '/', "parsing a closing tag slash");
            validate(consume(), '>', "parsing an closing tag close");

        } else {
            validate(consume(), '>', "parsing an tag close");

            children = parse_nodes();

            validate(consume(), '<', "parsing a closing tag open");
            validate(consume(), '/', "parsing a closing tag slash");
            VALIDATE(parse_tag_name() == tag_name, "parsing an tag name close");
            validate(consume(), '>', "parsing an closing tag close");
        }
        return SVGNode{.tag = tag_name, .attr = attrs, .children = children};
    }

    SVGNode parse_text() {
        log_error("parse text not implemented");
        return SVGNode();
    }

    SVGNode parse_node() {
        return next_char() == '<' ? parse_element() : parse_text();
    }

    SVGNodes parse_nodes() {
        SVGNodes nodes;
        int i = 0;
        // only 20 max siblings for now :)
        while (i++ < 20) {
            consume_whitespace();
            if (is_eof() || starts_with("</")) break;
            auto single_ = parse_node();
            nodes.push_back(single_);
        }
        return nodes;
    }
};

// TODO switch to the one in preload or something
#include <fstream>
#include <sstream>
#include <string>
inline std::string loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline SVGNode load_and_parse(const std::string& filename) {
    Parser p(loadFile(filename));
    SVGNodes nodes = p.parse_nodes();
    // TODO handle empty file
    // TODO validate certain elements
    return nodes[0];
}
