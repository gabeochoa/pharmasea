
#pragma once

#include <string>
#include <unordered_map>

#include "../preload.h"
#include "util.h"

// TODO is there a better way to do this?
//
// Because we want the UI to be as simple as possible to write,
// devs dont have to specify the font size since its dynamic based on size.
//
// This means that like size, its caculated every frame. In case this causes
// any rendering lag**, we need some way of caching the font sizes given a
// certiain (content + width + height)
//
// ** I never actually noticed any frame drop so this optimization was premature
//
// ****************************************************** start kludge

struct FZInfo {
    const std::string content;
    float width;
    float height;
    float spacing;
};

namespace std {

template<class T>
inline void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template<>
struct hash<FZInfo> {
    std::size_t operator()(const FZInfo& info) const {
        using std::hash;
        using std::size_t;
        using std::string;

        std::size_t out = 0;
        hash_combine(out, info.content);
        hash_combine(out, info.width);
        hash_combine(out, info.height);
        hash_combine(out, info.spacing);
        return out;
    }
};

}  // namespace std

inline bool operator==(const FZInfo& info, const FZInfo& other) {
    return std::hash<FZInfo>()(info) == std::hash<FZInfo>()(other);
}

// ****************************************************** end kludge

struct FontSizeCache {
    raylib::Font font;
    std::unordered_map<size_t, float> _font_size_memo;

    virtual void init() { this->set_font(Preload::get().font); }

    void set_font(raylib::Font f) { font = f; }

    [[nodiscard]] float get_font_size_impl(const std::string& content,
                                           float width, float height,
                                           float spacing) const {
        float font_size = 10.0f;
        float last_size = 10.0f;
        vec2 size;

        // NOTE: if you are looking at a way to speed this up switch to using
        // powers of two and theres no need to ceil or cast to int. also
        // shifting ( font_size <<= 1) is a clean way to do this
        //

        // lets see how big can we make the size before growing
        // outside our bounds
        do {
            last_size = font_size;
            // the smaller the number we multiply by (>1) the better fitting the
            // text will be
            font_size = ceilf(font_size * 1.05f);
            log_trace("measuring for {}", font_size);
            size = MeasureTextEx(font, content.c_str(), font_size, spacing);
            log_trace("got {},{} for {} and {},{} and last was: {}", size.x,
                      size.y, font_size, width, height, last_size);
        } while (size.x <= width && size.y <= height);

        // return the last one that passed
        return last_size;
    }

    [[nodiscard]] float get_font_size(const std::string& content, float width,
                                      float height, float spacing) {
        FZInfo fzinfo =
            FZInfo{.content = content, .width = width, .height = height};
        auto hash = std::hash<FZInfo>()(fzinfo);
        if (!_font_size_memo.contains(hash)) {
            _font_size_memo[hash] =
                get_font_size_impl(content, width, height, spacing);
        } else {
            log_trace("found value in cache");
        }
        float result = _font_size_memo[hash];
        log_trace("cache value for '{}' was {}", content, result);
        return result;
    }
};
