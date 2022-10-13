
#pragma once

#include "external_include.h"
#include "raylib.h"
#include "ui_color.h"
#include "uuid.h"

namespace ui {

enum SizeMode { Null, Pixels, Text, Percent, Children };
enum GrowDir { Row, Column };

struct SizeExpectation {
    SizeMode mode;
    float value;
    float strictness;
};

std::ostream& operator<<(std::ostream& os, const SizeExpectation& exp) {
    os << "SizeExpectation(";
    os << exp.mode;
    os << ", ";
    os << exp.value;
    os << ", ";
    os << exp.strictness;
    os << ")";
    return os;
}

struct Widget {
    SizeExpectation size_expected[2];
    float computed_relative_pos[2] = {0.f, 0.f};
    float computed_size[2] = {-1.f, -1.f};
    Rectangle rect;
    GrowDir growdir = GrowDir::Column;
    //
    Widget* first = nullptr;
    Widget* last = nullptr;
    Widget* prev = nullptr;
    Widget* next = nullptr;
    Widget* parent = nullptr;

    void add_child(Widget* child) {
        if (!this->first) {
            this->first = child;
            this->last = child;
            child->parent = this;
            return;
        }

        Widget* c = this->first;
        while (c->next) {
            c = c->next;
        }
        c->next = child;
        child->prev = c;
        child->parent = this;
        this->last = child;
    }
};

std::ostream& operator<<(std::ostream& os, const Widget& w) {
    os << "Widget(";
    os << w.size_expected[0] << "\n";
    os << w.size_expected[1] << "\n";
    os << "rel_pos(";
    os << w.computed_relative_pos[0];
    os << ", ";
    os << w.computed_relative_pos[1];
    os << ")\n";
    os << "computed size(";
    os << w.computed_size[0] << ", " << w.computed_size[1];
    os << ")\n";
    os << "Dir" << w.growdir << "\n";
    os << "First:" << w.first << "\n";
    os << "Last:" << w.last << "\n";
    os << "Prev:" << w.prev << "\n";
    os << "Next:" << w.next << "\n";
    os << "Parent:" << w.parent << "\n";
    return os;
}

}  // namespace ui
