
#pragma once

#include "external_include.h"
#include "globals.h"
#include "raylib.h"
#include "ui_color.h"
#include "uuid.h"
#include "vec_util.h"

namespace ui {

enum SizeMode { Null, Pixels, Text, Percent, Children };
enum GrowFlags { None = (1 << 0), Row = (1 << 1), Column = (1 << 2) };

struct SizeExpectation {
    SizeMode mode = Pixels;
    // TODO is there a better default?
    float value = 100.f;
    float strictness = 0.5f;
};

#define Size_Px(v, s) \
    { .mode = Pixels, .value = v, .strictness = s }

#define Size_Pct(v, s) \
    { .mode = Percent, .value = v, .strictness = s }

std::ostream& operator<<(std::ostream& os, const SizeExpectation& exp) {
    switch (exp.mode) {
        case Null:
            os << "Null";
            break;
        case Pixels:
            os << exp.value << " pixels";
            break;
        case Text:
            os << "Size of given text";
            break;
        case Percent:
            os << exp.value * 100.f << " percent";
            break;
        case Children:
            os << "Width of all children";
            break;
        default:
            os << "Missing Case for " << exp.mode;
            break;
    }
    os << " @ ";
    os << round(exp.strictness * 100.f);
    os << "%)";
    return os;
}

struct Widget {
    Widget* me;
    uuid id;
    std::string element = "";
    bool absolute = false;

    static void set_element(const Widget& widget, std::string e) {
        auto& w = const_cast<Widget&>(widget);
        w.element = e;
    }
    // Layout computed size stuffs
    SizeExpectation size_expected[2];
    float computed_relative_pos[2] = {0.f, 0.f};
    float computed_size[2] = {-1.f, -1.f};
    Rectangle rect;
    int growflags = GrowFlags::None;
    // Parent child stuffs
    std::vector<Widget*> children;
    Widget* parent = nullptr;

    bool cant_render() const {
        return computed_size[0] == -1.f || computed_size[1] == -1.f;
    }
    // TODO Support rotations

    uuid default_id() const { return static_cast<uuid>(MK_UUID(id, ROOT_ID)); }

    Widget() {
        this->id = default_id();
        this->me = this;
    }

    Widget(const Widget& other) {
        this->me = this;
        this->id = other.id;
        this->element = other.element;
        this->absolute = other.absolute;
        this->size_expected[0] = other.size_expected[0];
        this->size_expected[1] = other.size_expected[1];
        this->computed_relative_pos[0] = other.computed_relative_pos[0];
        this->computed_relative_pos[1] = other.computed_relative_pos[1];
        this->computed_size[0] = other.computed_size[0];
        this->computed_size[1] = other.computed_size[1];
        this->rect = other.rect;
        this->growflags = other.growflags;
        this->children = other.children;
        this->parent = other.parent;
    }

    Widget(SizeExpectation x, SizeExpectation y) {
        this->me = this;
        this->id = default_id();
        this->size_expected[0] = x;
        this->size_expected[1] = y;
    }

    Widget(const uuid& id, SizeExpectation x, SizeExpectation y) {
        this->me = this;
        this->id = id;
        this->size_expected[0] = x;
        this->size_expected[1] = y;
    }

    Widget(SizeExpectation x, SizeExpectation y, int flags) {
        this->me = this;
        this->id = default_id();
        this->size_expected[0] = x;
        this->size_expected[1] = y;
        this->growflags = flags;
    }

    Widget(const uuid& id, SizeExpectation x, SizeExpectation y,
           GrowFlags flags) {
        this->me = this;
        this->id = id;
        this->size_expected[0] = x;
        this->size_expected[1] = y;
        this->growflags = flags;
    }

    Widget& operator=(const Widget& other) {
        this->me = this;
        this->id = other.id;
        this->element = other.element;
        this->absolute = other.absolute;
        this->size_expected[0] = other.size_expected[0];
        this->size_expected[1] = other.size_expected[1];
        this->computed_relative_pos[0] = other.computed_relative_pos[0];
        this->computed_relative_pos[1] = other.computed_relative_pos[1];
        this->computed_size[0] = other.computed_size[0];
        this->computed_size[1] = other.computed_size[1];
        this->rect = other.rect;
        this->growflags = other.growflags;
        this->children = other.children;
        this->parent = other.parent;
        return *this;
    }

    void set_expectation(SizeExpectation x, SizeExpectation y) {
        this->size_expected[0] = x;
        this->size_expected[1] = y;
    }

    void add_child(Widget* child) {
        children.push_back(child);
        child->parent = this;
    }

    std::string print(int t = 0) const {
        std::stringstream ss;

        std::string tabs(t, '\t');

        const auto a = (this->parent ? this->parent->rect : Rectangle());
        ss << tabs << "Widget(" << this->element << " ";
        ss << "x(" << this->size_expected[0] << " ; y("
           << this->size_expected[1] << "\n";
        ss << tabs << "rel_pos(" << this->computed_relative_pos[0] << ", "
           << this->computed_relative_pos[1] << ") ";
        ss << "computed size(" << this->computed_size[0] << ", "
           << this->computed_size[1] << ")\n";
        ss << tabs << "rect(" << this->rect << ")\n";
        ss << tabs << "Dir " << this->growflags << "\n";
        ss << tabs << "Absolute " << this->absolute << "\n";
        ss << tabs << "Children:" << this->children.size()
           << " Parent:" << &(this->parent) << " " << a << "\n";
        return ss.str();
    }

    void print_tree(int t = 0) const {
        std::cout << me->print(t) << "\n";
        for (const Widget* child : children) {
            if (child) child->print_tree(t + 2);
        }
    }
};

struct ButtonConfig : public Widget {
    std::string text;
};

struct DropdownConfig : public Widget {
    bool vertical;
};

std::ostream& operator<<(std::ostream& os, const Widget& w) {
    os << w.print();
    return os;
}

}  // namespace ui
