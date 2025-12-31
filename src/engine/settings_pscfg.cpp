#include "settings_pscfg.h"

#include <bit>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "settings_schema.h"
#include "util.h"

namespace settings_pscfg {
namespace {

static uint32_t f32_bits(float v) { return std::bit_cast<uint32_t>(v); }

static float f32_from_bits(uint32_t bits) { return std::bit_cast<float>(bits); }

static bool f32_bits_equal(float a, float b) { return f32_bits(a) == f32_bits(b); }

class pscfg_parser {
   public:
    explicit pscfg_parser(std::string_view input) : in(input) {}

    void parse_file() {
        while (!eof()) {
            parse_stmt();
        }
    }

    [[nodiscard]] const std::vector<Assignment>& get_assignments() const {
        return assignments;
    }

    [[nodiscard]] const std::vector<Message>& get_messages() const { return messages; }

    [[nodiscard]] bool has_multiple_versions() const { return multiple_versions; }

    [[nodiscard]] const std::optional<int>& get_version() const { return version; }

   private:
    std::string_view in;
    size_t i = 0;
    int line = 1;

    std::string current_section;
    std::unordered_map<std::string, std::string> section_lower_to_original;

    std::optional<int> version;
    bool multiple_versions = false;

    std::vector<Assignment> assignments;
    std::vector<Message> messages;

    [[nodiscard]] bool eof() const { return i >= in.size(); }

    [[nodiscard]] char peek(size_t off = 0) const {
        const size_t p = i + off;
        if (p < in.size()) return in[p];
        return '\0';
    }

    char get() {
        char c = peek();
        if (!eof()) ++i;
        if (c == '\n') ++line;
        return c;
    }

    static bool is_ws(char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\0' ||
               c == '\v' || c == '\f';
    }

    void skip_to_eol() {
        while (!eof() && peek() != '\n') {
            (void) get();
        }
    }

    void skip_ws_and_comments() {
        while (!eof()) {
            if (is_ws(peek())) {
                (void) get();
                continue;
            }

            // Full-line and inline comments are treated the same: once we see a
            // comment token, ignore until end-of-line.
            if (peek() == '#') {
                skip_to_eol();
                continue;
            }

            if (peek() == '/' && peek(1) == '/') {
                (void) get();
                (void) get();
                skip_to_eol();
                continue;
            }

            break;
        }
    }

    void warn_here(std::string msg) {
        messages.push_back(Message{Message::Level::Warn, line, std::move(msg)});
    }

    bool consume(char expected) {
        skip_ws_and_comments();
        if (peek() != expected) return false;
        (void) get();
        return true;
    }

    // identifier := [a-zA-Z_][a-zA-Z0-9_]*
    std::optional<std::string> parse_ident() {
        skip_ws_and_comments();
        const char c = peek();
        if (!(std::isalpha((unsigned char) c) || c == '_')) {
            return std::nullopt;
        }

        std::string out;
        while (!eof()) {
            const char ch = peek();
            if (std::isalnum((unsigned char) ch) || ch == '_') {
                out.push_back(get());
            } else {
                break;
            }
        }
        return out;
    }

    std::optional<int32_t> parse_i32_raw() {
        skip_ws_and_comments();

        // Allow leading '-' only.
        if (peek() != '-' && !std::isdigit((unsigned char) peek())) {
            return std::nullopt;
        }

        const char* begin = in.data() + i;
        const char* end = in.data() + in.size();
        int32_t value = 0;

        auto res = std::from_chars(begin, end, value, 10);
        if (res.ec != std::errc()) {
            return std::nullopt;
        }

        i = (size_t) (res.ptr - in.data());
        return value;
    }

    std::optional<int> parse_int_raw() {
        auto v = parse_i32_raw();
        if (!v.has_value()) return std::nullopt;
        return (int) v.value();
    }

    std::optional<uint32_t> parse_hex_u32_exact8() {
        skip_ws_and_comments();
        if (!(peek() == '0' && (peek(1) == 'x' || peek(1) == 'X'))) {
            return std::nullopt;
        }
        (void) get();
        (void) get();

        uint32_t value = 0;
        int digits = 0;

        while (!eof()) {
            const char c = peek();
            int v = -1;

            if (c >= '0' && c <= '9')
                v = c - '0';
            else if (c >= 'a' && c <= 'f')
                v = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F')
                v = 10 + (c - 'A');
            else
                break;

            value = (value << 4) | (uint32_t) v;
            ++digits;
            (void) get();
        }

        if (digits != 8) return std::nullopt;
        return value;
    }

    std::optional<std::string> parse_quoted_string() {
        // Note: comment parsing must not trigger inside strings, so we do NOT
        // call skip_ws_and_comments() after consuming the opening quote.
        skip_ws_and_comments();
        if (peek() != '"') return std::nullopt;
        (void) get();

        std::string out;
        while (!eof()) {
            const char c = get();
            if (c == '"') return out;
            if (c == '\n' || c == '\0') return std::nullopt;

            if (c == '\\') {
                if (eof()) return std::nullopt;
                const char esc = get();
                switch (esc) {
                    case '"':
                        out.push_back('"');
                        break;
                    case '\\':
                        out.push_back('\\');
                        break;
                    case 'n':
                        out.push_back('\n');
                        break;
                    case 't':
                        out.push_back('\t');
                        break;
                    default:
                        // Minimal escape support: treat unknown escapes literally.
                        out.push_back(esc);
                        break;
                }
                continue;
            }

            out.push_back(c);
        }

        return std::nullopt;
    }

    std::optional<Assignment::Value> parse_literal() {
        skip_ws_and_comments();

        // bool
        if (peek() == 't' && in.substr(i).starts_with("true")) {
            i += 4;
            return Assignment::Value{true};
        }
        if (peek() == 'f' && in.substr(i).starts_with("false")) {
            i += 5;
            return Assignment::Value{false};
        }

        // i32(...)
        if (in.substr(i).starts_with("i32")) {
            i += 3;
            if (!consume('(')) return std::nullopt;
            auto v = parse_i32_raw();
            if (!v.has_value()) return std::nullopt;
            if (!consume(')')) return std::nullopt;
            return Assignment::Value{v.value()};
        }

        // str("...")
        if (in.substr(i).starts_with("str")) {
            i += 3;
            if (!consume('(')) return std::nullopt;
            auto s = parse_quoted_string();
            if (!s.has_value()) return std::nullopt;
            if (!consume(')')) return std::nullopt;
            return Assignment::Value{std::move(s.value())};
        }

        // f32(0xXXXXXXXX)
        if (in.substr(i).starts_with("f32")) {
            i += 3;
            if (!consume('(')) return std::nullopt;
            auto bits = parse_hex_u32_exact8();
            if (!bits.has_value()) return std::nullopt;
            if (!consume(')')) return std::nullopt;
            return Assignment::Value{f32_from_bits(bits.value())};
        }

        // i32x2(a, b)
        if (in.substr(i).starts_with("i32x2")) {
            i += 5;
            if (!consume('(')) return std::nullopt;
            auto a = parse_i32_raw();
            if (!a.has_value()) return std::nullopt;
            if (!consume(',')) return std::nullopt;
            auto b = parse_i32_raw();
            if (!b.has_value()) return std::nullopt;
            if (!consume(')')) return std::nullopt;
            return Assignment::Value{std::make_pair(a.value(), b.value())};
        }

        return std::nullopt;
    }

    void recover_to_statement_end() {
        // Skip until ';' or end-of-line; also consume the ';' if found.
        while (!eof()) {
            const char c = peek();
            if (c == ';') {
                (void) get();
                return;
            }
            if (c == '\n') {
                (void) get();
                return;
            }
            (void) get();
        }
    }

    void parse_version_stmt() {
        // Positioned after "version"
        if (!consume(':')) {
            warn_here("Malformed version statement; expected ':'");
            recover_to_statement_end();
            return;
        }

        auto v = parse_int_raw();
        if (!v.has_value()) {
            warn_here("Malformed version statement; expected integer version");
            recover_to_statement_end();
            return;
        }

        if (!consume(';')) {
            warn_here("Malformed version statement; expected ';'");
            recover_to_statement_end();
            return;
        }

        if (version.has_value()) {
            multiple_versions = true;
        }
        version = v.value();
    }

    void parse_section_stmt() {
        // Positioned at '['
        (void) get();

        std::string name;
        while (!eof()) {
            const char c = get();
            if (c == ']') break;
            if (c == '\n' || c == '\0') {
                warn_here("Malformed section header; missing ']'");
                return;
            }
            name.push_back(c);
        }

        current_section = name;

        const std::string lower = util::toLowerCase(current_section);
        const auto it = section_lower_to_original.find(lower);
        if (it == section_lower_to_original.end()) {
            section_lower_to_original.emplace(lower, current_section);
        } else if (it->second != current_section) {
            warn_here("Multiple section names differ only by case: '" + it->second +
                      "' vs '" + current_section + "'");
        }
    }

    void parse_assign_stmt(std::string key_ident) {
        Assignment a;
        if (!current_section.empty()) {
            a.section = current_section;
        }
        a.key = std::move(key_ident);
        a.line = line;

        // allow whitespace before '*'
        skip_ws_and_comments();
        if (peek() == '*') {
            a.starred = true;
            (void) get();
        }

        if (!consume('=')) {
            warn_here("Malformed assignment; expected '='");
            recover_to_statement_end();
            return;
        }

        auto lit = parse_literal();
        if (!lit.has_value()) {
            warn_here("Malformed assignment; bad or unsupported literal");
            recover_to_statement_end();
            return;
        }

        if (!consume(';')) {
            warn_here("Malformed assignment; expected ';'");
            recover_to_statement_end();
            return;
        }

        a.value = std::move(lit.value());
        assignments.push_back(std::move(a));
    }

    void parse_stmt() {
        skip_ws_and_comments();
        if (eof()) return;

        if (peek() == '[') {
            parse_section_stmt();
            return;
        }

        auto ident = parse_ident();
        if (!ident.has_value()) {
            // Unknown token; skip line.
            warn_here("Unrecognized token; skipping");
            skip_to_eol();
            return;
        }

        if (ident.value() == "version") {
            parse_version_stmt();
            return;
        }

        parse_assign_stmt(std::move(ident.value()));
    }
};

static bool value_is_bool(const Assignment::Value& v) {
    return std::holds_alternative<bool>(v);
}
static bool value_is_f32(const Assignment::Value& v) {
    return std::holds_alternative<float>(v);
}
static bool value_is_str(const Assignment::Value& v) {
    return std::holds_alternative<std::string>(v);
}
static bool value_is_i32x2(const Assignment::Value& v) {
    return std::holds_alternative<std::pair<int32_t, int32_t>>(v);
}

static std::string escape_str(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

static std::string fmt_f32_bits(float v) {
    const uint32_t bits = f32_bits(v);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "f32(0x%08X)", bits);
    return std::string(buf);
}

static std::string fmt_str(std::string_view s) {
    return "str(\"" + escape_str(s) + "\")";
}

static std::string fmt_i32x2(int32_t a, int32_t b) {
    return "i32x2(" + std::to_string(a) + ", " + std::to_string(b) + ")";
}

struct write_line {
    std::string section;
    std::string key;
    std::string literal;
};

static void push_override_bool(std::vector<write_line>& out, std::string section,
                               std::string key, bool value, bool def_value) {
    if (value == def_value) return;
    out.push_back(write_line{std::move(section), std::move(key),
                             value ? "true" : "false"});
}

static void push_override_str(std::vector<write_line>& out, std::string section,
                              std::string key, const std::string& value,
                              const std::string& def_value) {
    if (value == def_value) return;
    out.push_back(
        write_line{std::move(section), std::move(key), fmt_str(value)});
}

static void push_override_f32(std::vector<write_line>& out, std::string section,
                              std::string key, float value, float def_value) {
    if (f32_bits_equal(value, def_value)) return;
    out.push_back(
        write_line{std::move(section), std::move(key), fmt_f32_bits(value)});
}

static void push_override_i32x2(std::vector<write_line>& out, std::string section,
                                std::string key, int32_t a, int32_t b, int32_t da,
                                int32_t db) {
    if (a == da && b == db) return;
    out.push_back(write_line{std::move(section), std::move(key), fmt_i32x2(a, b)});
}

static bool section_has_any(const std::vector<write_line>& lines,
                            std::string_view section_name) {
    for (const auto& l : lines) {
        if (l.section == section_name) return true;
    }
    return false;
}

static void emit_section(std::ostringstream& os, const std::vector<write_line>& lines,
                         std::string_view section_name) {
    if (!section_has_any(lines, section_name)) return;

    os << "\n[" << section_name << "]\n";
    for (const auto& l : lines) {
        if (l.section != section_name) continue;
        os << l.key << "* = " << l.literal << ";\n";
    }
}

}  // namespace

LoadResult load_from_string(std::string_view input, const settings::Data& defaults,
                            int current_version) {
    LoadResult out;
    out.data = defaults;
    out.used_defaults = true;

    pscfg_parser parser(input);
    parser.parse_file();

    out.messages = parser.get_messages();

    if (parser.has_multiple_versions()) {
        out.messages.push_back(Message{
            Message::Level::Error,
            1,
            "Multiple version statements; treating settings file as invalid",
        });
        return out;
    }

    if (!parser.get_version().has_value()) {
        out.messages.push_back(Message{
            Message::Level::Error,
            1,
            "Missing required version statement; treating settings file as invalid",
        });
        return out;
    }

    if (parser.get_version().value() != current_version) {
        out.messages.push_back(Message{
            Message::Level::Warn,
            1,
            "Settings version mismatch; discarding file and using defaults",
        });
        return out;
    }

    // Apply overrides in order. Track duplicates ("last one wins").
    std::unordered_map<std::string, int> key_seen_count;
    bool warned_nonstar = false;

    const auto warn = [&](int warn_line, std::string msg) {
        out.messages.push_back(Message{Message::Level::Warn, warn_line, std::move(msg)});
    };

    const auto bump_dup = [&](const std::string& key, int warn_line) {
        int& n = key_seen_count[key];
        ++n;
        if (n > 1) warn(warn_line, "Duplicate key '" + key + "'; last one wins");
    };

    for (const auto& a : parser.get_assignments()) {
        if (!a.starred) {
            if (!warned_nonstar) {
                warned_nonstar = true;
                warn(a.line,
                     "Non-star assignments are treated as non-overrides and ignored");
            }
            continue;
        }

        // We currently ignore sections for key identity.
        bump_dup(a.key, a.line);

        const auto* spec = settings_schema::find_key(a.key);
        if (spec == nullptr) {
            warn(a.line, "Unknown key '" + a.key + "'; ignoring");
            continue;
        }

        // Lifecycle validation.
        if (current_version < spec->lifecycle.since) {
            warn(a.line, "Key '" + a.key + "' is not valid in this version; ignoring");
            continue;
        }
        if (spec->lifecycle.removed_in_version.has_value() &&
            current_version >= spec->lifecycle.removed_in_version.value()) {
            out.messages.push_back(Message{
                Message::Level::Error,
                a.line,
                "Key '" + a.key + "' is removed in this version; ignoring",
            });
            continue;
        }
        if (spec->lifecycle.deprecated_since.has_value() &&
            current_version >= spec->lifecycle.deprecated_since.value()) {
            warn(a.line, "Key '" + a.key + "' is deprecated");
        }

        // Type validation + apply.
        bool ok = false;
        if (spec->type == settings_schema::ValueType::Bool) {
            ok = value_is_bool(a.value) && spec->bool_member != nullptr;
            if (ok) out.data.*(spec->bool_member) = std::get<bool>(a.value);
        } else if (spec->type == settings_schema::ValueType::F32) {
            ok = value_is_f32(a.value) && spec->f32_member != nullptr;
            if (ok) out.data.*(spec->f32_member) = std::get<float>(a.value);
        } else if (spec->type == settings_schema::ValueType::Str) {
            ok = value_is_str(a.value) && spec->str_member != nullptr;
            if (ok) out.data.*(spec->str_member) = std::get<std::string>(a.value);
        } else if (spec->type == settings_schema::ValueType::I32x2) {
            ok = value_is_i32x2(a.value) && spec->set_i32x2 != nullptr;
            if (ok) {
                const auto [w, h] =
                    std::get<std::pair<int32_t, int32_t>>(a.value);
                spec->set_i32x2(out.data, w, h);
            }
        } else {
            ok = false;
        }

        if (!ok) {
            warn(a.line, "Type mismatch for '" + a.key + "'; keeping default");
            continue;
        }
    }

    out.data.engineVersion = current_version;
    out.used_defaults = false;
    return out;
}

std::string write_overrides_only(const settings::Data& current,
                                 const settings::Data& defaults,
                                 int current_version) {
    std::vector<write_line> lines;
    lines.reserve(32);

    // Canonical ordering: follow schema list order.
    for (const auto& spec : settings_schema::all_keys()) {
        // Don't write keys that aren't valid in the current version.
        if (current_version < spec.lifecycle.since) continue;
        if (spec.lifecycle.removed_in_version.has_value() &&
            current_version >= spec.lifecycle.removed_in_version.value()) {
            continue;
        }

        if (spec.type == settings_schema::ValueType::Bool) {
            if (spec.bool_member == nullptr) continue;
            push_override_bool(lines, std::string(spec.section),
                               std::string(spec.key), current.*(spec.bool_member),
                               defaults.*(spec.bool_member));
        } else if (spec.type == settings_schema::ValueType::F32) {
            if (spec.f32_member == nullptr) continue;
            push_override_f32(lines, std::string(spec.section),
                              std::string(spec.key), current.*(spec.f32_member),
                              defaults.*(spec.f32_member));
        } else if (spec.type == settings_schema::ValueType::Str) {
            if (spec.str_member == nullptr) continue;
            push_override_str(lines, std::string(spec.section),
                              std::string(spec.key), current.*(spec.str_member),
                              defaults.*(spec.str_member));
        } else if (spec.type == settings_schema::ValueType::I32x2) {
            if (spec.get_i32x2 == nullptr) continue;
            int32_t a = 0, b = 0;
            int32_t da = 0, db = 0;
            spec.get_i32x2(current, a, b);
            spec.get_i32x2(defaults, da, db);
            push_override_i32x2(lines, std::string(spec.section),
                                std::string(spec.key), a, b, da, db);
        }
    }

    std::ostringstream os;
    os << "version: " << current_version << ";\n";

    emit_section(os, lines, "audio");
    emit_section(os, lines, "video");
    emit_section(os, lines, "ui");
    emit_section(os, lines, "network");

    return os.str();
}

}  // namespace settings_pscfg

