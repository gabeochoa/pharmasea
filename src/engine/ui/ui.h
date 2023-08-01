
#pragma once

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../strings.h"
#include "../assert.h"
#include "../log.h"
#include "../ui.h"
#include "../util.h"

// TODO switch to the one in preload or something
inline std::string loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct Node;

typedef std::pair<std::string, std::string> Attr;
typedef std::unordered_map<std::string, std::string> Attrs;
typedef std::vector<Node> Nodes;

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
};

// TODO eventually support selectors
typedef std::map<std::string, Rule> RuleMap;

struct Node {
    std::string content;
    std::string tag;
    Attrs attrs;
    Nodes children;
    Node(){};

    std::optional<Style> style;

    explicit Node(const std::string& content) : content(content) {}
    Node(const std::string& tag, const Attrs& attrs, const Nodes& children)
        : tag(tag), attrs(attrs), children(children) {}
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

struct Parser {
    size_t pos;
    std::string input;

    Parser(const std::string& source) : input(source) { pos = 0; }

    void validate(unsigned char c, unsigned char m, const std::string& msg) {
        VALIDATE(c == m, fmt::format("{} != {} =>  {}", c, m, msg));
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

    std::string consume_while(std::function<bool(unsigned char)> pred) {
        std::string out;
        while (!is_eof() && pred(next_char())) {
            out.push_back(consume());
        }
        return out;
    }

    std::string parse_tag_name() {
        return consume_while([](unsigned char c) { return std::isalnum(c); });
    }

    Node parse_text() {
        return dom::text(
            consume_while([](unsigned char c) { return c != '<'; }));
    }

    void consume_whitespace() {
        consume_while([](unsigned char c) { return std::isspace(c); });
    }

    void consume_singleline_comment() {
        if (!starts_with("//")) return;
        consume_while([](unsigned char c) { return c != '\n'; });
    }

    void consume_multiline_comment() {
        if (!starts_with("/*")) return;
        consume_while([](unsigned char c) { return c != '*'; });
        consume();  // *
        consume();  // /
    }
};

struct CSSParser : Parser {
    CSSParser(const std::string& source) : Parser(source) {
        // log_info("css parser: {}", source);
    }

    std::string parse_style_field() {
        return consume_while(
            [](unsigned char c) { return std::isalnum(c) || c == '-'; });
    }

    std::string parse_style_value() {
        return consume_while([](unsigned char c) { return c != ';'; });
    }

    Decl parse_decl() {
        consume_whitespace();
        // name: value;
        auto field = parse_style_field();
        validate(consume(), ':', "should be a colon between field and value");
        auto value = parse_style_value();
        validate(consume(), ';', "should end with a semi colon");
        return Decl{
            .field = field,
            .value = value,
        };
    }

    Rule parse_rule() {
        Decls decls;

        int i = 0;
        while (i++ < 50 /* max decls */) {
            consume_whitespace();
            if (is_eof() || next_char() == '}') break;
            decls.push_back(parse_decl());
        }

        return Rule{.decls = decls};
    }

    RuleMap parse_rule_map() {
        RuleMap rule_map;
        int i = 0;
        while (i++ < 50 /* max rules */) {
            consume_whitespace();
            if (next_char() == '<' || is_eof()) break;

            //
            {
                auto tag = parse_tag_name();
                consume_whitespace();
                validate(consume(), '{', "should be open rule {");
                Rule r = parse_rule();
                validate(consume(), '}', "should be close rule {");
                rule_map.insert({tag, r});
            }
        }
        return rule_map;
    }
};

struct HTMLParser : Parser {
    HTMLParser(const std::string& source) : Parser(source) {}

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
            if (next_char() == '>') break;
            auto p = parse_attr();
            attrs.insert(p);
        }
        return attrs;
    }

    Nodes parse_nodes() {
        Nodes nodes;
        int i = 0;
        while (i++ < 20 /* max siblings */) {
            consume_whitespace();
            if (is_eof() || starts_with("</")) break;
            auto sib = parse_node();
            nodes.push_back(sib);
        }
        return nodes;
    }

    Node parse_element() {
        validate(consume(), '<', "parsing an tag open");
        auto tag_name = parse_tag_name();
        auto attrs = parse_attrs();
        validate(consume(), '>', "parsing an tag close");

        auto children = parse_nodes();

        validate(consume(), '<', "parsing a closing tag open");
        validate(consume(), '/', "parsing a closing tag slash");
        VALIDATE(parse_tag_name() == tag_name, "parsing an tag name close");
        validate(consume(), '>', "parsing an closing tag close");

        return dom::elem(tag_name, attrs, children);
    }

    Node parse_node() {
        return next_char() == '<' ? parse_element() : parse_text();
    }

    std::optional<Node> find_style_node(const Node& root) {
        if (root.tag.empty()) return {};
        if (root.tag == "style") return root.children[0];

        for (auto child : root.children) {
            auto c = find_style_node(child);
            if (c.has_value()) return c;
        }
        return {};
    }

    float parse_decl_value(const std::string& value) {
        Parser p(value);
        p.consume_whitespace();
        auto num = p.consume_while(
            [](unsigned char c) { return std::isdigit(c) || c == '.'; });
        // TODO consume and check units
        // TODO for now just divide by 100
        return (std::stof(num) / 100.f);
    }

    void populate_style(Style& style, const Decl& decl) {
        float value = parse_decl_value(decl.value);
        switch (hashString(decl.field)) {
            case hashString("width"): {
                style.width = value;
            } break;
            case hashString("height"): {
                style.height = value;
            } break;
        }
    }

    void apply_rules(Node& root, const RuleMap& rules) {
        // Not appling css to raw text
        if (root.tag.empty()) return;

        // Apply to parent first since they might be relative
        for (const auto& r : rules) {
            auto matching_tag = r.first;
            if (matching_tag != root.tag) continue;
            Style style;
            for (const auto& decl : r.second.decls) {
                populate_style(style, decl);
            }
            root.style = style;
        }

        for (auto& child : root.children) {
            apply_rules(child, rules);
        }
    }

    void load_css(Nodes& nodes) {
        std::optional<Node> style = find_style_node(nodes[0]);
        if (!style.has_value()) {
            log_warn("was not able to find css style node");
            return;
        }
        CSSParser p(style.value().content);
        RuleMap rule_map = p.parse_rule_map();
        apply_rules(nodes[0], rule_map);
    }
};

inline Node parse(const std::string& source) {
    HTMLParser p(source);
    Nodes nodes = p.parse_nodes();
    p.load_css(nodes);
    return nodes.size() == 1 ? nodes[0] : dom::elem("html", {}, nodes);
}

inline void dump(const Node& root, int indent = 0) {
    std::cout << std::string(indent, ' ') << root << std::endl;
    for (auto& child : root.children) {
        dump(child, indent + 4);
    }
}

inline Node load_and_parse(const std::string& filename) {
    return parse(loadFile(filename));
}

inline int ui_main() {
    // Load the UI markup and CSS styles from files or strings
    Node root = load_and_parse("resources/html/simple.html");
    dump(root);

    return 0;
}

inline void render_node(std::shared_ptr<ui::UIContext> ui_context,
                        const Node& root,
                        raylib::Rectangle parent = raylib::Rectangle{0, 0, 1280,
                                                                     720}) {
    using namespace ui;

    if (root.tag.empty()) {
        ui_context->_draw_text(parent, text_lookup(root.content.c_str()),
                               ui::theme::Usage::Font);
        return;
    }

    Style style = root.style.has_value() ? root.style.value()
                                         : Style{
                                               .width = 1.f,
                                               .height = 1.f,
                                           };

    auto p = raylib::Rectangle{
        0, 0,                         //
        style.width * parent.width,   //
        style.height * parent.height  //
    };

    switch (hashString(root.tag)) {
        case hashString("em"):
        case hashString("div"):
            ui_context->draw_widget_rect(p, theme::Usage::Primary);
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
                     root.tag);
            return;
    }

    for (const auto& child : root.children) {
        render_node(ui_context, child, p);
    }
}
