
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
    float value = fminf(WIN_H, WIN_W);
    float strictness = 0.1f;
};

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
            os << exp.value << " percent";
            break;
        case Children:
            os << "Width of all children";
            break;
        default:
            os << "Missing Case for " << exp.mode;
            break;
    }
    os << " @ ";
    os << exp.strictness;
    os << ")";
    return os;
}

struct Widget {
    Widget* me;
    uuid id;
    std::string element = "";

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
    bool cant_render() const {
        return computed_size[0] == -1.f || computed_size[1] == -1.f;
    }
    // TODO Support rotations

    uuid default_id() const { return MK_UUID(id, ROOT_ID); }

    Widget() {
        this->id = default_id();
        this->me = this;
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

    Widget(SizeExpectation x, SizeExpectation y, GrowFlags flags) {
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

    void set_expectation(SizeExpectation x, SizeExpectation y) {
        this->size_expected[0] = x;
        this->size_expected[1] = y;
    }

    // Parent child stuffs
    std::vector<Widget*> children;
    Widget* parent = nullptr;

    void add_child(Widget* child) {
        children.push_back(child);
        child->parent = this;
    }

    std::string print() const {
        std::stringstream ss;

        const auto a = (this->parent ? this->parent->rect : Rectangle());

        ss << "Widget(" << this->element << "\n";
        ss << "x(" << this->size_expected[0] << " ; ";
        ss << "y(" << this->size_expected[1] << "\n";
        ss << "rel_pos(";
        ss << this->computed_relative_pos[0];
        ss << ", ";
        ss << this->computed_relative_pos[1];
        ss << ")\n";
        ss << "computed size(";
        ss << this->computed_size[0] << ", " << this->computed_size[1];
        ss << ")\n";
        ss << "rect(" << this->rect << ")\n";
        ss << "Dir " << this->growflags << "\n";
        ss << "Children:" << this->children.size() << " ";
        ss << "Parent:" << &(this->parent) << " " << a << "\n";
        return ss.str();
    }

    void print_tree() const {
        std::cout << me->print() << std::endl;
        std::cout << "start children for " << this << std::endl;
        for (const Widget* child : children) {
            if (child) child->print_tree();
        }
        std::cout << "end children for " << this << std::endl;
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
