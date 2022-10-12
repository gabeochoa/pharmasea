
#pragma once

#include "assert.h"
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

// Process order
//
// - (any) compute solos (doesnt rely on parent/child / other widgets)
// - (pre) parent sizes
// - (post) children
// - (pre) solve violations
// - (pre) compute relative positions
/*
 pre order me -> left -> right
 post order left -> right -> me

 (Any order is acceptable) Calculate “standalone” sizes. These are sizes that do
not depend on other widgets and can be calculated purely with the information
that comes from the single widget that is having its size calculated.
(UI_SizeKind_Pixels, UI_SizeKind_TextContent)

(Pre-order) Calculate “upwards-dependent” sizes. These are sizes that strictly
depend on an ancestor’s size, other than ancestors that have
“downwards-dependent” sizes on the given axis. (UI_SizeKind_PercentOfParent)

(Post-order) Calculate “downwards-dependent” sizes. These are sizes that depend
on sizes of descendants. (UI_SizeKind_ChildrenSum)

(Pre-order) Solve violations. For each level in the hierarchy, this will verify
that the children do not extend past the boundaries of a given parent (unless
explicitly allowed to do so; for example, in the case of a parent that is
scrollable on the given axis), to the best of the algorithm’s ability. If there
is a violation, it will take a proportion of each child widget’s size (on the
given axis) proportional to both the size of the violation, and (1-strictness),
where strictness is that specified in the semantic size on the child widget for
the given axis.

(Pre-order) Finally, given the calculated sizes of each widget, compute the
relative positions of each widget (by laying out on an axis which can be
specified on any parent node). This stage can also compute the final
screen-coordinates rectangle.
 * */

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

std::ostream& operator<<(std::ostream& os, const Widget& w) {
    os << "Widget(";
    os << w.size_expected[0];
    os << "\n";
    os << w.size_expected[1];
    os << "\n";
    os << "rel_pos(";
    os << w.computed_relative_pos[0];
    os << ", ";
    os << w.computed_relative_pos[1];
    os << ")";
    os << "\n";
    os << "computed size(";
    os << w.computed_size[0];
    os << ", ";
    os << w.computed_size[1];
    os << ")";
    os << "\n";
    os << "Dir";
    os << w.growdir;
    os << "\n";
    os << "First:" << w.first << "\n";
    os << "Last:" << w.last << "\n";
    os << "Prev:" << w.prev << "\n";
    os << "Next:" << w.next << "\n";
    os << "Parent:" << w.parent << "\n";
    return os;
}

float compute_size_for_standalone_expectation(Widget* widget, int exp_index) {
    // std::cout << "csfse" << widget << " " << exp_index << std::endl;
    SizeExpectation exp = widget->size_expected[exp_index];
    switch (exp.mode) {
        case SizeMode::Pixels:
            return exp.value;
        case SizeMode::Text:
            // TODO figure this out
            return 100.f;
        default:
            return widget->computed_size[exp_index];
    }
}

float compute_size_for_parent_expectation(Widget* widget, int exp_index) {
    // std::cout << "csfpe" << widget << " " << exp_index << std::endl;
    float no_change = widget->computed_size[exp_index];
    if (!widget->parent) return no_change;
    float parent_size = widget->parent->computed_size[exp_index];
    SizeExpectation exp = widget->size_expected[exp_index];
    switch (exp.mode) {
        case SizeMode::Percent:
            return parent_size == -1 ? no_change : exp.value * parent_size;
        default:
            return no_change;
    }
}

float compute_size_for_child_expectation(Widget* widget, int exp_index) {
    // std::cout << "csfce" << widget << " " << exp_index << std::endl;
    float no_change = widget->computed_size[exp_index];
    if (!widget->first) return no_change;

    float total_child_size = 0.f;
    Widget* child = widget->first;
    while (child) {
        // TODO are children guaranteed to have been solved by now?
        float cs = child->computed_size[exp_index];
        M_ASSERT(cs != -1, "expect that all children have been solved by now");
        total_child_size += cs;
        child = child->next;
    }

    SizeExpectation exp = widget->size_expected[exp_index];
    switch (exp.mode) {
        case SizeMode::Children:
            return total_child_size;
        default:
            return no_change;
    }
}

void calculate_standalone(Widget* widget) {
    if (widget == nullptr) return;

    auto size_x = compute_size_for_standalone_expectation(widget, 0);
    auto size_y = compute_size_for_standalone_expectation(widget, 1);

    widget->computed_size[0] = size_x;
    widget->computed_size[1] = size_y;

    Widget* child = widget->first;
    while (child) {
        calculate_standalone(child);
        child = child->next;
    }
}

void calculate_those_with_parents(Widget* widget) {
    if (widget == nullptr) return;

    auto size_x = compute_size_for_parent_expectation(widget, 0);
    auto size_y = compute_size_for_parent_expectation(widget, 1);
    widget->computed_size[0] = size_x;
    widget->computed_size[1] = size_y;

    Widget* child = widget->first;
    while (child) {
        calculate_those_with_parents(child);
        child = child->next;
    }
}

void calculate_those_with_children(Widget* widget) {
    if (widget == nullptr) return;

    Widget* child = widget->first;
    while (child) {
        calculate_those_with_children(child);
        child = child->next;
    }

    auto size_x = compute_size_for_child_expectation(widget, 0);
    auto size_y = compute_size_for_child_expectation(widget, 1);
    widget->computed_size[0] = size_x;
    widget->computed_size[1] = size_y;
}

float _get_total_child_size(Widget* widget, int exp_index) {
    Widget* child = widget->first;
    float sum = 0.f;
    while (child) {
        sum += child->computed_size[exp_index];
        child = child->next;
    }
    return sum;
}

int _get_num_children(Widget* widget) {
    int i = 0;
    Widget* child = widget->first;
    while (child) {
        i++;
        child = child->next;
    }
    return i;
}

void fix_violating_children(Widget* widget, int exp_index, float error,
                            int num_children) {
    M_ASSERT(num_children != 0, "Should never have zero children");
    float approx_epc = error / num_children;
    Widget* child = widget->first;
    while (child) {
        SizeExpectation exp = child->size_expected[exp_index];
        float portion_of_error = (1.f - exp.strictness) * approx_epc;
        // std::cout << "hi" << exp.strictness << std::endl;
        if (exp.strictness == 0.f) {
            // If 0 then accept all the error
            portion_of_error = error;
            float cur_size = child->computed_size[exp_index];
            child->computed_size[exp_index] = cur_size - portion_of_error;
            break;
        } else if (exp.strictness == 1.f) {
            // If 1.f, then we dont want to touch it
        } else {
            float cur_size = child->computed_size[exp_index];
            child->computed_size[exp_index] = cur_size - portion_of_error;
            // Reduce strictness every round
            // so that eventually we will get there
            exp.strictness = fmaxf(0.f, exp.strictness - 0.1f);
            child->size_expected[exp_index] = exp;
        }
        // next iteration
        child = child->next;
    }
}

void solve_violations(Widget* widget) {
    int num_children = _get_num_children(widget);
    if(num_children == 0) return;

    // me -> left -> right

    float my_size_x = widget->computed_size[0];
    float my_size_y = widget->computed_size[1];

    float all_children_x = _get_total_child_size(widget, 0);
    float error_x = all_children_x - my_size_x;
    while (error_x > 0.5f) {
        // std::cout << "current error" << error_x << std::endl;
        fix_violating_children(widget, 0, error_x, num_children);
        all_children_x = _get_total_child_size(widget, 0);
        error_x = all_children_x - my_size_x;
    }

    float all_children_y = _get_total_child_size(widget, 1);
    float error_y = all_children_y - my_size_y;
    while (error_y > 0.5f) {
        fix_violating_children(widget, 1, error_y, num_children);
        all_children_y = _get_total_child_size(widget, 1);
        error_y = all_children_y - my_size_y;
    }

    // Solve for children
    Widget* child = widget->first;
    while (child) {
        solve_violations(child);
        child = child->next;
    }
}

void compute_relative_positions(Widget* widget) {
    float sx = widget->computed_size[0];
    float sy = widget->computed_size[1];

    float offx = 0.f;
    float offy = 0.f;

    float col_w = 0.f;
    float col_h = 0.f;

    float max_child_w = 0.f;
    float max_child_h = 0.f;

    Widget* child = widget->first;
    while (child) {
        float cx = child->computed_size[0];
        float cy = child->computed_size[1];
        if (cx > max_child_w) max_child_w = cx;
        if (cy > max_child_h) max_child_h = cy;
        child = child->next;
    }

    switch (widget->growdir) {
        case GrowDir::Column:
            col_h = sy;
            col_w = max_child_w;
            break;
        case GrowDir::Row:
            col_w = sx;
            col_h = max_child_h;
            break;
    }

    child = widget->first;
    while (child) {
        float cx = child->computed_size[0];
        float cy = child->computed_size[1];

        switch (widget->growdir) {
            case GrowDir::Column:
                // hit max height, wrap
                if (cy + offy > sy) {
                    offy = 0;
                    offx += col_w;
                }
                child->computed_relative_pos[0] = offx;
                child->computed_relative_pos[1] = offy;
                break;
            case GrowDir::Row:
                // hit max width, wrap
                if (cx + offx > sx) {
                    offx = 0;
                    offy += col_h;
                }
                child->computed_relative_pos[0] = offx;
                child->computed_relative_pos[1] = offy;
                break;
        }
        compute_relative_positions(child);
        child = child->next;
    }
}

void process_widget(Widget* widget) {
    // - (any) compute solos (doesnt rely on parent/child / other widgets)
    calculate_standalone(widget);
    // - (pre) parent sizes
    calculate_those_with_parents(widget);
    // - (post) children
    calculate_those_with_children(widget);
    // - (pre) solve violations
    solve_violations(widget);
    // - (pre) compute relative positions
    compute_relative_positions(widget);
}

}  // namespace ui
