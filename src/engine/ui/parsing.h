#pragma once

#include <cctype>

#include "../assert.h"
#include "node.h"
#include "utils.h"

struct Parser {
    // TODO replace with U+FFFD
    unsigned char REPLACEMENT_CHAR = ' ';

    enum State {
        CharacterReference,
        Data,

        RawText,
        RawTextLessThan,
        RawTextEndTagOpen,
        RawTextEndTagName,

        RCData,
        RCDataLessThan,
        RCDataEndTagOpen,
        RCDataEndTagName,

        Plaintext,
        MarkupDeclarationOpen,

        TagName,
        TagOpen,
        EndTagOpen,

        BogusComment,
        BeforeAttributeName,
        SelfClosingStartTag,

        ScriptData,
        ScriptDataEndTagOpen,
        ScriptDataEndTagName,
        ScriptDataEscaped,
        ScriptDataEscapedStart,
        ScriptDataEscapedDash,
        ScriptDataEscapedLessThan,
        ScriptDataEscapedEndTagOpen,
        ScriptDataEscapedEndTagName,
        ScriptDataEscapedStartDash,
        ScriptDataEscapedDashDash,
        ScriptDataDoubleEscaped,
        ScriptDataDoubleEscapedDash,
        ScriptDataDoubleEscapedDashDash,
        ScriptDataDoubleEscapedLessThan,
        ScriptDataDoubleEscapedEnd,

        //  TODO continue from
        //  https://html.spec.whatwg.org/#before-attribute-name-state
    } state = Data;

    size_t pos;
    std::string input;

    struct Token {
        enum Type { EndTag, StartTag, Character, EndOfFile, Comment } type;
        std::string data;
    };

    State return_state = Data;
    std::vector<Token> tokens;
    std::vector<Token> emitted;
    std::string temp_buffer = "";

    Parser(const std::string& source) : input(source) { pos = 0; }

    unsigned char next_char() const {
        if (is_eof()) return -1;
        return input[pos];
    }
    bool is_eof() const { return pos >= input.size(); }
    unsigned char consume() { return input[pos++]; }
    void do_not_consume() { return; }

    void emit(unsigned char c) {
        emitted.push_back(
            Token{.type = Token::Character, .data = std::string(1, c)});
    }
    void emit(const Token& token) { emitted.push_back(token); }
    void emit(const Token::Type& type) {
        emitted.push_back(Token{.type = type});
    }

    void create(const Token& token) { tokens.push_back(token); }

    void data_state() {
        if (is_eof()) {
            emit(Token::EndOfFile);
            return;
        }
        auto nc = consume();

        switch (nc) {
            case '&': {
                return_state = Data;
                state = CharacterReference;
                return;
            } break;
            case '<': {
                state = TagOpen;
                return;
            } break;
            case 0: {
                log_warn("unexpected null parse error");
                emit(nc);
            } break;
            default:
                emit(nc);
                break;
        }
    }

    void rc_data_state() {
        if (is_eof()) {
            emit(Token::EndOfFile);
            return;
        }
        auto nc = consume();
        switch (nc) {
            case '&': {
                return_state = RCData;
                state = CharacterReference;
                return;
            } break;
            case '<': {
                state = RCDataLessThan;
                return;
            } break;
            case 0: {
                log_warn("unexpected null parse error");
                // TODO emit U+FFFD Replacement Character
                emit(0);
            } break;
            default:
                emit(nc);
                break;
        }
    }

    void raw_text_state() {
        if (is_eof()) {
            emit(Token::EndOfFile);
            return;
        }
        auto nc = consume();
        switch (nc) {
            case '<': {
                state = RawTextLessThan;
                return;
            } break;
            case 0: {
                log_warn("unexpected null parse error");
                // TODO emit U+FFFD Replacement Character
                emit(0);
            } break;
            default:
                emit(nc);
                break;
        }
    }

    void script_data_state() {
        if (is_eof()) {
            emit(Token::EndOfFile);
            return;
        }
        auto nc = consume();
        switch (nc) {
            case '<': {
                state = RawTextLessThan;
                return;
            } break;
            case 0: {
                log_warn("unexpected null parse error");
                emit(REPLACEMENT_CHAR);
            } break;
            default:
                emit(nc);
                break;
        }
    }

    void plaintext_state() {
        auto nc = next_char();

        if (is_eof()) {
            emit(Token::EndOfFile);
            return;
        }

        if (nc == 0) {
            consume();

            log_warn("unexpected null parse error");
            emit(REPLACEMENT_CHAR);
        }

        // anything else
        consume();
        emit(nc);
    }

    // https://html.spec.whatwg.org/#tag-open-state
    void tag_open_state() {
        auto nc = next_char();

        if (nc == '!') {
            consume();
            state = MarkupDeclarationOpen;
            return;
        }

        if (nc == '/') {
            consume();
            state = EndTagOpen;
            return;
        }

        if (std::isalpha(nc)) {
            do_not_consume();

            create({.type = Token::StartTag, .data = ""});
            state = TagName;
            return;
        }

        if (nc == '?') {
            do_not_consume();

            log_warn("unexpected question mark instead of tag name error");
            create({.type = Token::Comment, .data = ""});
            state = BogusComment;
            return;
        }

        if (is_eof()) {
            consume();

            log_warn("invalid first character of tag name error");
            emit('<');
            emit(Token::EndOfFile);
            return;
        }

        // Anything else

        do_not_consume();
        log_warn("invalid first character of tag name error");
        emit('<');
        state = Data;
    }

    void end_tag_open_state() {
        auto nc = next_char();

        if (std::isalpha(nc)) {
            do_not_consume();

            create({.type = Token::EndTag, .data = ""});
            state = TagName;
            return;
        }

        if (nc == '>') {
            consume();
            log_warn("missing end tag name parse error");
            state = Data;
            return;
        }

        if (is_eof()) {
            consume();

            log_warn("eof before tag name error");
            emit('<');
            emit('/');
            emit(Token::EndOfFile);
            return;
        }

        // Anything else

        do_not_consume();
        log_warn("invalid first character of tag name error");
        create({.type = Token::Comment, .data = ""});
        state = BogusComment;
    }

    void tag_name_state() {
        auto nc = next_char();

        if (                            //
            nc == '\t' ||               //
            nc == 12 /* line feed*/ ||  //
            nc == 14 /* form feed*/ ||  //
            nc == ' '                   //
        ) {
            consume();
            state = BeforeAttributeName;
            return;
        }

        if (nc == '/') {
            consume();
            state = SelfClosingStartTag;
            return;
        }

        if (nc == '>') {
            consume();

            state = Data;
            emit(nc);
            return;
        }

        if (std::isupper(nc)) {
            consume();

            // lower case it
            tokens.back().data.push_back(nc + 32);
            return;
        }

        if (nc == 0) {
            consume();

            log_warn("unexpected null character");
            tokens.back().data.push_back(REPLACEMENT_CHAR);
            return;
        }

        if (is_eof()) {
            consume();
            emit(Token::EndOfFile);
            return;
        }

        // Anything else

        consume();
        tokens.back().data.push_back(nc);
    }

    bool is_appropriate_end_tag_token() {
        // An appropriate end tag token is an end tag token whose tag name
        // matches the tag name of the last start tag to have been emitted from
        // this tokenizer, if any.

        if (tokens.back().type != Token::EndTag) {
            log_warn("looking for app but isnt an end tag, need to search");
        }

        auto it = std::find_if(
            emitted.rbegin(), emitted.rend(),
            [](const Token& token) { return token.type == Token::StartTag; });
        // If no start tag has been emitted from this tokenizer,
        // then no end tag token is appropriate.
        if (it == emitted.rend()) {
            return false;
        }
        // Does the token match the last emitted start tag?
        return tokens.back().data == it->data;
    }

    void shared_end_tag_name(State anything_else) {
        auto nc = next_char();

        const auto _anything_else = [&]() {
            do_not_consume();
            emit('<');
            emit('/');
            for (auto c : temp_buffer) {
                emit(c);
            }
            state = anything_else;
        };

        if (                            //
            nc == '\t' ||               //
            nc == 12 /* line feed*/ ||  //
            nc == 14 /* form feed*/ ||  //
            nc == ' '                   //
        ) {
            // if current end tag token is appropriate,
            if (is_appropriate_end_tag_token()) {
                consume();
                state = BeforeAttributeName;
                return;
            }

            // otherwise treat is as per anything
            _anything_else();
            return;
        }

        if (nc == '/') {
            if (is_appropriate_end_tag_token()) {
                consume();
                state = SelfClosingStartTag;
                return;
            }
            // otherwise treat is as per anything
            _anything_else();
            return;
        }

        if (nc == '>') {
            if (is_appropriate_end_tag_token()) {
                consume();
                state = Data;
                emit(tokens.back());
                tokens.pop_back();
                return;
            }
            // otherwise treat is as per anything
            _anything_else();
            return;
        }

        if (std::isupper(nc)) {
            consume();

            // lower case it
            tokens.back().data.push_back(nc + 32);
            temp_buffer.push_back(nc);
            return;
        }

        if (std::islower(nc)) {
            consume();

            tokens.back().data.push_back(nc);
            temp_buffer.push_back(nc);
            return;
        }

        // anything else
        _anything_else();
        return;
    }

    void rc_data_end_tag_name() { shared_end_tag_name(RCData); }
    void script_data_end_tag_name() { shared_end_tag_name(ScriptData); }
    void raw_text_end_tag_name() { shared_end_tag_name(RawText); }

    void shared_end_tag_open(State alpha, State anything_else) {
        auto nc = next_char();
        if (std::isalpha(nc)) {
            do_not_consume();
            create({.type = Token::EndTag, .data = ""});
            state = alpha;
            return;
        }

        do_not_consume();
        emit('<');
        emit('/');
        state = anything_else;
    }

    void raw_text_end_tag_open() {
        shared_end_tag_open(RawTextEndTagName, RawText);
    }
    void rc_data_end_tag_open() {
        shared_end_tag_open(RCDataEndTagName, RCData);
    }
    void script_data_end_tag_open() {
        shared_end_tag_open(ScriptDataEndTagName, ScriptData);
    }

    void shared_less_than(State slash, State anything_else) {
        auto nc = next_char();
        if (nc == '/') {
            consume();
            temp_buffer.clear();
            state = slash;
            return;
        }

        do_not_consume();
        emit('<');
        state = anything_else;
    }

    void raw_text_less_than() { shared_less_than(RawTextEndTagOpen, RawText); }
    void rc_data_less_than() { shared_less_than(RCDataEndTagOpen, RCData); }
    void script_data_less_than() {
        auto nc = next_char();
        if (nc == '!') {
            consume();

            state = ScriptDataEscapedStart;
            emit('<');
            emit('!');
            return;
        }

        shared_less_than(ScriptDataEndTagOpen, ScriptData);
    }

    void script_data_escape_start() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataEscapedStartDash;
            return;
        }

        // Anything else
        do_not_consume();
        state = ScriptData;
    }

    void script_data_escape_start_dash() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataEscapedDashDash;
            return;
        }

        // Anything else
        do_not_consume();
        state = ScriptData;
    }

    void script_data_escaped() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataEscapedDash;
            return;
        }

        if (nc == '<') {
            consume();
            state = ScriptDataEscapedLessThan;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }
        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        return;
    }

    void script_data_escaped_dash() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataEscapedDashDash;
            return;
        }

        if (nc == '<') {
            consume();
            state = ScriptDataEscapedLessThan;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }
        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        state = ScriptDataEscaped;
        return;
    }

    void script_data_escaped_dash_dash() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            return;
        }

        if (nc == '<') {
            consume();
            state = ScriptDataEscapedLessThan;
            return;
        }

        if (nc == '>') {
            consume();
            emit('>');
            state = ScriptDataEscapedLessThan;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }
        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        state = ScriptDataEscaped;
        return;
    }

    void script_data_escaped_less_than() {
        auto nc = next_char();

        if (nc == '/') {
            consume();
            temp_buffer.clear();
            state = ScriptDataEscapedEndTagOpen;
            return;
        }

        if (std::isalpha(nc)) {
            do_not_consume();
            temp_buffer.clear();
            emit('<');
            state = ScriptDataDoubleEscaped;
            return;
        }

        do_not_consume();
        emit('<');
        state = ScriptDataEscaped;
    }

    void script_data_escaped_end_tag_open() {
        auto nc = next_char();

        if (std::isalpha(nc)) {
            do_not_consume();

            create({.type = Token::EndTag, .data = ""});
            state = ScriptDataEscapedEndTagName;
            return;
        }

        do_not_consume();
        emit('<');
        state = ScriptDataEscaped;
    }

    void script_data_escaped_end_tag_name() {
        shared_end_tag_name(ScriptDataEscaped);
    }

    void script_data_double_escape_start() {
        auto nc = next_char();

        if (                            //
            nc == '\t' ||               //
            nc == 12 /* line feed*/ ||  //
            nc == 14 /* form feed*/ ||  //
            nc == ' ' ||                //
            nc == '/' ||                //
            nc == '>'                   //
        ) {
            consume();

            if (temp_buffer == "script") {
                state = ScriptDataDoubleEscaped;
            } else {
                state = ScriptDataEscaped;
            }
            emit(nc);
            return;
        }

        if (std::isupper(nc)) {
            consume();

            // lower case it
            temp_buffer.push_back(nc + 32);
            emit(nc);
            return;
        }

        if (std::islower(nc)) {
            consume();

            // lower case it
            temp_buffer.push_back(nc);
            emit(nc);
            return;
        }

        do_not_consume();
        state = ScriptDataEscaped;
    }

    void script_data_double_escaped() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataDoubleEscapedDash;
            return;
        }

        if (nc == '<') {
            consume();
            emit('<');
            state = ScriptDataDoubleEscapedLessThan;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }
        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        return;
    }

    void script_data_double_escaped_dash() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            state = ScriptDataDoubleEscapedDashDash;
            return;
        }

        if (nc == '<') {
            consume();
            emit('<');
            state = ScriptDataDoubleEscapedLessThan;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }

        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        return;
    }

    void script_data_double_escaped_dash_dash() {
        auto nc = next_char();

        if (nc == '-') {
            consume();
            emit('-');
            return;
        }

        if (nc == '<') {
            consume();
            emit('<');
            state = ScriptDataDoubleEscapedLessThan;
            return;
        }

        if (nc == '>') {
            consume();
            emit('>');
            state = ScriptData;
            return;
        }

        if (nc == 0) {
            consume();
            emit(REPLACEMENT_CHAR);
            return;
        }

        if (is_eof()) {
            log_warn("eof in script html comment like text parse error");
            emit(Token::EndOfFile);
            return;
        }

        // Anything else
        consume();
        emit(nc);
        state = ScriptDataDoubleEscaped;
        return;
    }

    void script_data_double_escaped_less_than() {
        auto nc = next_char();
        if (nc == '/') {
            consume();
            emit('/');
            temp_buffer.clear();
            state = ScriptDataDoubleEscapedEnd;
            return;
        }

        do_not_consume();
        state = ScriptDataDoubleEscaped;
    }

    void script_data_double_escaped_end() {
        auto nc = next_char();

        if (                            //
            nc == '\t' ||               //
            nc == 12 /* line feed*/ ||  //
            nc == 14 /* form feed*/ ||  //
            nc == ' ' ||                //
            nc == '/' ||                //
            nc == '>'                   //
        ) {
            if (temp_buffer == "script") {
                state = ScriptDataEscaped;
            } else {
                state = ScriptDataDoubleEscaped;
            }
            emit(nc);
            return;
        }

        if (std::isupper(nc)) {
            consume();
            temp_buffer.push_back(nc + 32);
            emit(nc);
            return;
        }

        if (std::islower(nc)) {
            consume();

            temp_buffer.push_back(nc);
            emit(nc);
            return;
        }

        do_not_consume();
        state = ScriptDataDoubleEscaped;
        return;
    }

    void tokenize() {
        switch (state) {
            case Data: {
                data_state();
            } break;
            case RCData: {
                rc_data_state();
            } break;
            case RawText: {
                raw_text_state();
            } break;
            case ScriptData: {
                script_data_state();
            } break;
            case Plaintext: {
                plaintext_state();
            } break;
            case TagOpen: {
                tag_open_state();
            } break;
            case CharacterReference:
            case RCDataLessThan:
            case RawTextLessThan:
                break;
        }
    }
};

struct ParserOld {
    size_t pos;
    std::string input;

    ParserOld(const std::string& source) : input(source) { pos = 0; }

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
        return consume_while([](unsigned char c) {
            return std::isalnum(c) || c == '.' || c == '-';
        });
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

    bool is_numerical_property(const std::string& property) {
        std::array<std::string, 17> num_prop{
            //
            "width",        "height",         "padding-left",  "padding-right",
            "padding-top",  "padding-bottom", "padding",       "margin-left",
            "margin-right", "margin-top",     "margin-bottom", "margin",
            "border-left",  "border-right",   "border-top",    "border-bottom",
            "border",
        };
        for (auto s : num_prop) {
            if (s == property) return true;
        }
        return false;
    }

    bool is_color_property(const std::string& property) {
        std::array<std::string, 1> color_prop{
            //
            "background-color",
        };
        for (auto s : color_prop) {
            if (s == property) return true;
        }
        return false;
    }

    Value parse_decl_numerical_value(const std::string& value) {
        Parser p(value);
        p.consume_whitespace();
        auto num = p.consume_while(
            [](unsigned char c) { return std::isdigit(c) || c == '.'; });

        NumericalValue::Unit unit;
        switch (p.next_char()) {
            case '%':
                unit = NumericalValue::Unit::Percentage;
                break;
            case 'p':
            case ';':
                unit = NumericalValue::Unit::Pixels;
                break;
            default:
                unit = NumericalValue::Unit::Pixels;
                break;
        }

        return NumericalValue{
            .data = (std::stof(num)),
            .unit = unit,
        };
    }

    Value parse_decl_color_value(const std::string& value) {
        Parser p(value);
        p.consume_whitespace();
        auto color = p.consume_while(
            [](unsigned char c) { return std::isalpha(c) || c == '"'; });

        auto color_no_quote = color.substr(1, color.length() - 2);

        return magic_enum::enum_cast<ui::theme::Usage>(color_no_quote)
            .value_or(ui::theme::Usage::Primary);
    }

    Value parse_decl_value(const std::string& property,
                           const std::string& value) {
        if (is_numerical_property(property))
            return parse_decl_numerical_value(value);
        if (is_color_property(property)) return parse_decl_color_value(value);
        auto val =
            (value[0] == '"') ? value.substr(1, value.length() - 2) : value;
        log_info("returning decl '{}' value '{}' as string", property, val);
        return val;
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
        consume_whitespace();
        Value value = parse_decl_value(
            field, consume_while([](unsigned char c) { return c != ';'; }));
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

    void ignore_doctype() {
        validate(consume(), '<', "parsing an tag open");
        if (next_char() == '!') {
            consume_while([](unsigned char c) { return c != '>'; });
        } else {
            pos--;
        }
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
        Style style;
        for (const auto& r : rules) {
            auto selector = r.first;
            bool match_one = (matches_tag(selector, root) ||  //
                              matches_class(selector, root));
            if (!match_one) continue;
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

    // first check for doctype
    p.ignore_doctype();

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
