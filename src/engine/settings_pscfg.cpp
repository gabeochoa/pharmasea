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

namespace settings_pscfg {
namespace {

static uint32_t f32_bits(float v) {
    return std::bit_cast<uint32_t>(v);
}

static float f32_from_bits(uint32_t bits) {
    return std::bit_cast<float>(bits);
}

static bool f32_bits_equal(float a, float b) {
    return f32_bits(a) == f32_bits(b);
}

static std::string to_lower(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out.push_back((char) std::tolower((unsigned char) c));
    return out;
}

struct Parser {
    std::string_view in;
    size_t i = 0;
    int line = 1;

    std::string current_section;
    std::unordered_map<std::string, std::string> section_lower_to_original;

    std::optional<int> version;
    bool multiple_versions = false;

    std::vector<Assignment> assignments;
    std::vector<Message> messages;

    bool eof() const { return i >= in.size(); }

    char peek(size_t off = 0) const {
        const size_t p = i + off;
        return p < in.size() ? in[p] : '\0';
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
        while (!eof() && peek() != '\n') (void) get();
    }

    void skip_ws_and_comments() {
        while (!eof()) {
            if (is_ws(peek())) {
                (void) get();
                continue;
            }

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
        messages.push_back(
            Message{Message::Level::Warn, line, std::move(msg)});
    }

    void error_here(std::string msg) {
        messages.push_back(
            Message{Message::Level::Error, line, std::move(msg)});
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
        char c = peek();
        if (!(std::isalpha((unsigned char) c) || c == '_')) return std::nullopt;

        std::string out;
        while (!eof()) {
            c = peek();
            if (std::isalnum((unsigned char) c) || c == '_') {
                out.push_back(get());
            } else {
                break;
            }
        }
        return out;
    }

    std::optional<int32_t> parse_i32_raw() {
        skip_ws_and_comments();

        const char* begin = in.data() + i;
        const char* end = in.data() + in.size();

        // Allow leading '-' only.
        if (peek() != '-' && !std::isdigit((unsigned char) peek())) return std::nullopt;

        int32_t value = 0;
        auto res = std::from_chars(begin, end, value, 10);
        if (res.ec != std::errc()) return std::nullopt;
        i = (size_t) (res.ptr - in.data());
        return value;
    }

    std::optional<int> parse_int_raw() {
        auto v = parse_i32_raw();
        if (!v.has_value()) return std::nullopt;
        return (int) v.value();
    }

    std::optional<uint32_t> parse_hex_u32() {
        skip_ws_and_comments();
        if (!(peek() == '0' && (peek(1) == 'x' || peek(1) == 'X'))) return std::nullopt;
        (void) get();
        (void) get();

        uint32_t value = 0;
        int digits = 0;
        while (!eof()) {
            char c = peek();
            int v = -1;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') v = 10 + (c - 'A');
            else break;

            value = (value << 4) | (uint32_t) v;
            ++digits;
            (void) get();
        }

        if (digits != 8) return std::nullopt;
        return value;
    }

    std::optional<std::string> parse_quoted_string() {
        skip_ws_and_comments();
        if (peek() != '"') return std::nullopt;
        (void) get();

        std::string out;
        while (!eof()) {
            char c = get();
            if (c == '"') return out;
            if (c == '\n' || c == '\0') return std::nullopt;
            if (c == '\\') {
                if (eof()) return std::nullopt;
                char esc = get();
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
            auto bits = parse_hex_u32();
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
        // We are positioned after "version".
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
        // We are positioned at '['
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
        const std::string lower = to_lower(current_section);
        const auto it = section_lower_to_original.find(lower);
        if (it == section_lower_to_original.end()) {
            section_lower_to_original.emplace(lower, current_section);
        } else if (it->second != current_section) {
            warn_here("Multiple section names differ only by case: '" +
                      it->second + "' vs '" + current_section + "'");
        }
    }

    void parse_assign_stmt(std::string key_ident) {
        Assignment a;
        a.section = current_section;
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

}  // namespace

LoadResult load_from_string(std::string_view input, const settings::Data& defaults,
                            int current_version) {
    LoadResult out;
    out.data = defaults;
    out.used_defaults = true;

    Parser p;
    p.in = input;

    while (!p.eof()) {
        p.parse_stmt();
    }

    // Parser warnings first.
    out.messages = std::move(p.messages);

    if (p.multiple_versions) {
        out.messages.push_back(Message{
            Message::Level::Error,
            1,
            "Multiple version statements; treating settings file as invalid",
        });
        return out;
    }

    if (!p.version.has_value()) {
        out.messages.push_back(Message{
            Message::Level::Error,
            1,
            "Missing required version statement; treating settings file as invalid",
        });
        return out;
    }

    if (p.version.value() != current_version) {
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

    auto warn = [&](int line, std::string msg) {
        out.messages.push_back(
            Message{Message::Level::Warn, line, std::move(msg)});
    };

    const auto bump_dup = [&](const std::string& key, int line) {
        int& n = key_seen_count[key];
        ++n;
        if (n > 1) {
            warn(line, "Duplicate key '" + key + "'; last one wins");
        }
    };

    for (const auto& a : p.assignments) {
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

        // Manual schema mapping (Phase 1/2: C++ is authoritative).
        if (a.key == "master_volume") {
            if (!value_is_f32(a.value)) {
                warn(a.line, "Type mismatch for 'master_volume'; keeping default");
                continue;
            }
            out.data.master_volume = std::get<float>(a.value);
            continue;
        }
        if (a.key == "music_volume") {
            if (!value_is_f32(a.value)) {
                warn(a.line, "Type mismatch for 'music_volume'; keeping default");
                continue;
            }
            out.data.music_volume = std::get<float>(a.value);
            continue;
        }
        if (a.key == "sound_volume") {
            if (!value_is_f32(a.value)) {
                warn(a.line, "Type mismatch for 'sound_volume'; keeping default");
                continue;
            }
            out.data.sound_volume = std::get<float>(a.value);
            continue;
        }
        if (a.key == "lang_name") {
            if (!value_is_str(a.value)) {
                warn(a.line, "Type mismatch for 'lang_name'; keeping default");
                continue;
            }
            out.data.lang_name = std::get<std::string>(a.value);
            continue;
        }
        if (a.key == "ui_theme") {
            if (!value_is_str(a.value)) {
                warn(a.line, "Type mismatch for 'ui_theme'; keeping default");
                continue;
            }
            out.data.ui_theme = std::get<std::string>(a.value);
            continue;
        }
        if (a.key == "username") {
            if (!value_is_str(a.value)) {
                warn(a.line, "Type mismatch for 'username'; keeping default");
                continue;
            }
            out.data.username = std::get<std::string>(a.value);
            continue;
        }
        if (a.key == "last_ip_joined") {
            if (!value_is_str(a.value)) {
                warn(a.line, "Type mismatch for 'last_ip_joined'; keeping default");
                continue;
            }
            out.data.last_ip_joined = std::get<std::string>(a.value);
            continue;
        }
        if (a.key == "show_streamer_safe_box") {
            if (!value_is_bool(a.value)) {
                warn(a.line,
                     "Type mismatch for 'show_streamer_safe_box'; keeping default");
                continue;
            }
            out.data.show_streamer_safe_box = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "enable_postprocessing") {
            if (!value_is_bool(a.value)) {
                warn(a.line,
                     "Type mismatch for 'enable_postprocessing'; keeping default");
                continue;
            }
            out.data.enable_postprocessing = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "enable_lighting") {
            if (!value_is_bool(a.value)) {
                warn(a.line,
                     "Type mismatch for 'enable_lighting'; keeping default");
                continue;
            }
            out.data.enable_lighting = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "snap_camera_to_90") {
            if (!value_is_bool(a.value)) {
                warn(a.line,
                     "Type mismatch for 'snap_camera_to_90'; keeping default");
                continue;
            }
            out.data.snapCameraTo90 = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "is_fullscreen") {
            if (!value_is_bool(a.value)) {
                warn(a.line, "Type mismatch for 'is_fullscreen'; keeping default");
                continue;
            }
            out.data.isFullscreen = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "vsync_enabled") {
            if (!value_is_bool(a.value)) {
                warn(a.line, "Type mismatch for 'vsync_enabled'; keeping default");
                continue;
            }
            out.data.vsync_enabled = std::get<bool>(a.value);
            continue;
        }
        if (a.key == "resolution") {
            if (!value_is_i32x2(a.value)) {
                warn(a.line, "Type mismatch for 'resolution'; keeping default");
                continue;
            }
            const auto [w, h] = std::get<std::pair<int32_t, int32_t>>(a.value);
            out.data.resolution.width = (int) w;
            out.data.resolution.height = (int) h;
            continue;
        }

        warn(a.line, "Unknown key '" + a.key + "'; ignoring");
    }

    out.data.engineVersion = current_version;
    out.used_defaults = false;
    return out;
}

std::string write_overrides_only(const settings::Data& current,
                                 const settings::Data& defaults,
                                 int current_version) {
    struct Line {
        std::string section;
        std::string key;
        std::string literal;
    };

    std::vector<Line> lines;
    lines.reserve(32);

    auto add_bool = [&](std::string section, std::string key, bool v,
                        bool def) {
        if (v == def) return;
        lines.push_back(Line{std::move(section), std::move(key),
                             v ? "true" : "false"});
    };

    auto add_str = [&](std::string section, std::string key,
                       const std::string& v, const std::string& def) {
        if (v == def) return;
        lines.push_back(
            Line{std::move(section), std::move(key), fmt_str(v)});
    };

    auto add_f32 = [&](std::string section, std::string key, float v,
                       float def) {
        if (f32_bits_equal(v, def)) return;
        lines.push_back(
            Line{std::move(section), std::move(key), fmt_f32_bits(v)});
    };

    auto add_i32x2 = [&](std::string section, std::string key, int32_t a,
                         int32_t b, int32_t da, int32_t db) {
        if (a == da && b == db) return;
        lines.push_back(Line{std::move(section), std::move(key),
                             fmt_i32x2(a, b)});
    };

    // Canonical ordering by section then key (manual; stable & deterministic).
    add_f32("audio", "master_volume", current.master_volume,
            defaults.master_volume);
    add_f32("audio", "music_volume", current.music_volume, defaults.music_volume);
    add_f32("audio", "sound_volume", current.sound_volume, defaults.sound_volume);

    add_bool("video", "is_fullscreen", current.isFullscreen, defaults.isFullscreen);
    add_i32x2("video", "resolution", (int32_t) current.resolution.width,
              (int32_t) current.resolution.height,
              (int32_t) defaults.resolution.width,
              (int32_t) defaults.resolution.height);
    add_bool("video", "vsync_enabled", current.vsync_enabled, defaults.vsync_enabled);
    add_bool("video", "enable_postprocessing", current.enable_postprocessing,
             defaults.enable_postprocessing);
    add_bool("video", "enable_lighting", current.enable_lighting,
             defaults.enable_lighting);
    add_bool("video", "snap_camera_to_90", current.snapCameraTo90,
             defaults.snapCameraTo90);

    add_str("ui", "lang_name", current.lang_name, defaults.lang_name);
    add_str("ui", "ui_theme", current.ui_theme, defaults.ui_theme);
    add_bool("ui", "show_streamer_safe_box", current.show_streamer_safe_box,
             defaults.show_streamer_safe_box);

    add_str("network", "username", current.username, defaults.username);
    add_str("network", "last_ip_joined", current.last_ip_joined,
            defaults.last_ip_joined);

    std::ostringstream os;
    os << "version: " << current_version << ";\n";

    // Emit in section groups; only emit a section if it has overrides.
    auto emit_section = [&](std::string_view section_name) {
        bool any = false;
        for (const auto& l : lines) {
            if (l.section == section_name) {
                any = true;
                break;
            }
        }
        if (!any) return;

        os << "\n[" << section_name << "]\n";
        for (const auto& l : lines) {
            if (l.section != section_name) continue;
            os << l.key << "* = " << l.literal << ";\n";
        }
    };

    emit_section("audio");
    emit_section("video");
    emit_section("ui");
    emit_section("network");

    return os.str();
}

}  // namespace settings_pscfg

