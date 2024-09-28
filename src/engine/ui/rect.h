
#pragma once

#include "raylib.h"

//

namespace rect {

inline Rectangle lpad(const Rectangle& r, float pct);
inline Rectangle rpad(const Rectangle& r, float pct);
inline Rectangle tpad(const Rectangle& r, float pct);
inline Rectangle bpad(const Rectangle& r, float pct);

inline raylib::Rectangle expand(const raylib::Rectangle& a, const vec4& b) {
    return (Rectangle){a.x - b.x,            //
                       a.y - b.y,            //
                       a.width + b.x + b.z,  //
                       a.height + b.y + b.w};
}

inline Rectangle expand_pct(const Rectangle& a, float pct) {
    return expand(a, {
                         a.width * pct,
                         a.width * pct,
                         a.height * pct,
                         a.height * pct,
                     });
}

inline Rectangle expand_px(const Rectangle& a, float px) {
    return expand(a, {px, px, px, px});
}

inline Rectangle offset_y(const Rectangle& a, float y) {
    return Rectangle{a.x,      //
                     a.y + y,  //
                     a.width,  //
                     a.height};
}

inline std::array<Rectangle, 2> vsplit(const Rectangle& a, float pct,
                                       float padding_right = 0) {
    float ratio = pct / 100.f;
    Rectangle left = {a.x, a.y, a.width * ratio, a.height};
    if (padding_right > 0) {
        left = rpad(left, 100 - padding_right);
    }
    Rectangle right = {a.x + (a.width * ratio), a.y, a.width * (1.f - ratio),
                       a.height};
    return {left, right};
}

inline std::array<Rectangle, 2> hsplit(const Rectangle& a, float pct,
                                       float padding_bottom = 0) {
    float ratio = pct / 100.f;
    Rectangle top = {a.x, a.y, a.width, a.height * ratio};
    if (padding_bottom > 0) {
        top = bpad(top, 100 - padding_bottom);
    }
    Rectangle bottom = {a.x, a.y + (a.height * ratio), a.width,
                        a.height * (1.f - ratio)};
    return {top, bottom};
}

inline Rectangle lpad(const Rectangle& r, float pct) {
    auto [_, b] = vsplit(r, pct);
    return b;
}

inline Rectangle rpad(const Rectangle& r, float pct) {
    auto [a, _] = vsplit(r, pct);
    return a;
}

inline Rectangle tpad(const Rectangle& r, float pct) {
    auto [_, b] = hsplit(r, pct);
    return b;
}

inline Rectangle bpad(const Rectangle& r, float pct) {
    auto [a, _] = hsplit(r, pct);
    return a;
}

inline Rectangle vpad(const Rectangle& r, float pct) {
    return tpad(bpad(r, 100 - pct), pct);
}

inline Rectangle hpad(const Rectangle& r, float pct) {
    return rpad(lpad(r, pct), 100 - pct);
}

inline Rectangle all_pad(const Rectangle& r, float pct) {
    return hpad(vpad(r, pct), pct);
}

template<size_t N>
inline std::array<Rectangle, N> vsplit(const Rectangle& a,
                                       float padding_right = 0) {
    std::array<Rectangle, N> rectangles;
    float step = a.width / N;

    float x = a.x;
    float y = a.y;
    float width = step;
    float height = a.height;
    for (size_t i = 0; i < N; ++i) {
        rectangles[i] = Rectangle{x, y, width, height};
        if (padding_right > 0) {
            rectangles[i] = rect::rpad(rectangles[i], 100 - padding_right);
        }
        x += step;
        y += 0;
    }
    return rectangles;
}

template<>
inline std::array<Rectangle, 2> vsplit(const Rectangle& a,
                                       float padding_right) {
    return vsplit(a, 50.f, padding_right);
}

template<size_t N>
inline std::array<Rectangle, N> hsplit(const Rectangle& a,
                                       float padding_bottom = 0) {
    std::array<Rectangle, N> rectangles;
    float step = a.height / N;

    float x = a.x;
    float y = a.y;
    float width = a.width;
    float height = step;
    for (size_t i = 0; i < N; ++i) {
        rectangles[i] = Rectangle{x, y, width, height};
        if (padding_bottom > 0) {
            rectangles[i] = rect::bpad(rectangles[i], 100 - padding_bottom);
        }
        x += 0;
        y += step;
    }
    return rectangles;
}

template<>
inline std::array<Rectangle, 2> hsplit(const Rectangle& a,
                                       float padding_bottom) {
    return hsplit(a, 50.f, padding_bottom);
}

}  // namespace rect
