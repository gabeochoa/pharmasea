#pragma once

#include "../assert.h"
#include "node.h"
#include "utils.h"

struct Parser {
    size_t pos;
    std::string input;

    Parser(const std::string& source) : input(source) { pos = 0; }

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

    std::string parse_attr_name() {
        return consume_while(
            [](unsigned char c) { return std::isalnum(c) || c == '.'; });
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

    Value parse_decl_value(const std::string& value) {
        Parser p(value);
        p.consume_whitespace();
        auto num = p.consume_while(
            [](unsigned char c) { return std::isdigit(c) || c == '.'; });

        Value::Unit unit;
        switch (p.next_char()) {
            case '%':
                unit = Value::Unit::Percentage;
                break;
            case 'p':
            case ';':
                unit = Value::Unit::Pixels;
                break;
            default:
                unit = Value::Unit::Pixels;
                break;
        }

        return {
            .data = (std::stof(num)),
            .unit = unit,
        };
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

    Decl parse_decl() {
        consume_whitespace();
        // name: value;
        auto field = parse_style_field();
        validate(consume(), ':', "should be a colon between field and value");
        Value value = parse_decl_value(
            consume_while([](unsigned char c) { return c != ';'; }));
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
                auto attr = parse_attr_name();
                consume_whitespace();
                validate(consume(), '{', "should be open rule {");
                Rule r = parse_rule();
                validate(consume(), '}', "should be close rule {");
                rule_map.insert({attr, r});
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

    void populate_style(Style& style, const Decl& decl) {
        style.values[decl.field] = decl.value;
    }

    bool matches_class(const std::string& selector, const Node& root) const {
        if (selector[0] != '.') return false;

        if (root.attrs.contains("class")) {
            return selector.substr(1) == root.attrs.at("class");
        }
        return false;
    }

    bool matches_tag(const std::string& selector, const Node& root) const {
        return selector == root.tag;
    }

    void apply_rules(Node& root, const RuleMap& rules) {
        // Not applying css to raw text
        if (root.tag.empty()) return;

        // Apply to parent first since they might be relative
        for (const auto& r : rules) {
            auto selector = r.first;
            bool match_one = (matches_tag(selector, root) ||  //
                              matches_class(selector, root));
            if (!match_one) continue;
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
