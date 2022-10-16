
#pragma once

#include "assert.h"
#include "external_include.h"
#include "ui_widget.h"
#include "vec_util.h"

namespace ui {
namespace autolayout {

const float ACCEPTABLE_ERROR = 0.5f;

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
    // std::cout << "csfce" << *widget << " " << exp_index << std::endl;
    float no_change = widget->computed_size[exp_index];
    if (widget->children.empty()) return no_change;

    float total_child_size = 0.f;
    for (Widget* child : widget->children) {
        float cs = child->computed_size[exp_index];
        if (cs == -1) {
            if (child->size_expected[exp_index].mode == SizeMode::Percent &&
                widget->size_expected[exp_index].mode == SizeMode::Children) {
                M_ASSERT(false,
                         "Parents sized with mode 'children' cannot have "
                         "children sized with mode 'percent'.");
            }
            // Instead of silently ignoring this, check the cases above
            M_ASSERT(false, "expect that all children have been solved by now");
        }
        total_child_size += cs;
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

    for (Widget* child : widget->children) {
        calculate_standalone(child);
    }
}

void calculate_those_with_parents(Widget* widget) {
    if (widget == nullptr) return;

    auto size_x = compute_size_for_parent_expectation(widget, 0);
    auto size_y = compute_size_for_parent_expectation(widget, 1);
    widget->computed_size[0] = size_x;
    widget->computed_size[1] = size_y;

    for (Widget* child : widget->children) {
        calculate_those_with_parents(child);
    }
}

void calculate_those_with_children(Widget* widget) {
    if (widget == nullptr) return;

    for (Widget* child : widget->children) {
        calculate_those_with_children(child);
    }

    auto size_x = compute_size_for_child_expectation(widget, 0);
    auto size_y = compute_size_for_child_expectation(widget, 1);
    widget->computed_size[0] = size_x;
    widget->computed_size[1] = size_y;
}

float _get_total_child_size(Widget* widget, int exp_index) {
    float sum = 0.f;
    for (Widget* child : widget->children) {
        sum += child->computed_size[exp_index];
    }
    return sum;
}

void fix_violating_children(Widget* widget, int exp_index, float error,
                            int num_children) {
    M_ASSERT(num_children != 0, "Should never have zero children");
    //

    // std::vector<std::vector<Widget*>::iterator>
    // proxy(widget->children.size()); for(auto iter = widget->children.begin();
    // iter != widget->children.end();
    // ++iter){
    // proxy.push_back(iter);
    // }
    // std::sort(proxy.begin(), proxy.end(), [&](auto it, auto it2) {
    // return (*it)->size_expected[exp_index].strictness <
    // (*it2)->size_expected[exp_index].strictness;
    // });
    //
    // for(auto iter : proxy){
    // SizeExpectation exp = (*iter)->size_expected[exp_index];
    // if(exp.strictness == 0.f){
    //
    // }
    // }

    int num_optional_children = 0;
    int num_absolute_children = 0;
    int num_ignorable_children = 0;
    for (Widget* child : widget->children) {
        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 0.f) {
            num_optional_children++;
        }
        if (exp.strictness == 1.f) {
            num_absolute_children++;
        }
        if (child->ignore_size) {
            num_ignorable_children++;
        }
    }

    // Solve error for all optional children
    if (num_optional_children != 0) {
        float approx_epc = error / num_optional_children;
        for (Widget* child : widget->children) {
            SizeExpectation exp = child->size_expected[exp_index];
            if (exp.strictness == 0.f) {
                float cur_size = child->computed_size[exp_index];
                child->computed_size[exp_index] = fmaxf(0, cur_size - approx_epc);
                if(cur_size > approx_epc) error -= approx_epc;
            }
        }
        if (error <= ACCEPTABLE_ERROR) {
            return;
        }
    }

    // If there is any error left,
    // we have to take away from the normal children;

    int num_resizeable_children = num_children - num_absolute_children -
                                  num_optional_children -
                                  num_ignorable_children;

    // TODO we cannot enforce the assert below in the case of wrapping
    // because the positioning happens after error correction
    if (error > ACCEPTABLE_ERROR && num_resizeable_children == 0 &&
        widget->growflags == GrowFlags::None) {
        M_ASSERT(
            num_resizeable_children > 0,
            "Cannot fit all children inside parent and unable to resize any of "
            "the children");
    }

    float approx_epc = error / (num_resizeable_children + num_optional_children);
    for (Widget* child : widget->children) {
        SizeExpectation exp = child->size_expected[exp_index];
        float portion_of_error = (1.f - exp.strictness) * approx_epc;
        if (exp.strictness == 1.f || child->ignore_size) {
            // If 1.f, then we dont want to touch it
        } else {
            float cur_size = child->computed_size[exp_index];
            child->computed_size[exp_index] = fmaxf(0, cur_size - portion_of_error);
            // Reduce strictness every round
            // so that eventually we will get there
            exp.strictness = fmaxf(0.f, exp.strictness - 0.1f);
            child->size_expected[exp_index] = exp;
        }
    }
}

void solve_violations(Widget* widget) {
    int num_children = widget->children.size();
    if (num_children == 0) return;

    // me -> left -> right

    float my_size_x = widget->computed_size[0];
    float my_size_y = widget->computed_size[1];

    float all_children_x = _get_total_child_size(widget, 0);
    float error_x = all_children_x - my_size_x;
    int i_x = 0;
    while (error_x > ACCEPTABLE_ERROR) {
        i_x++;
        // std::cout << "errorx" << error_x << std::endl;
        fix_violating_children(widget, 0, error_x, num_children);
        all_children_x = _get_total_child_size(widget, 0);
        error_x = all_children_x - my_size_x;
        if (i_x > 100) break;
    }

    float all_children_y = _get_total_child_size(widget, 1);
    float error_y = all_children_y - my_size_y;
    int i_y = 0;
    while (error_y > ACCEPTABLE_ERROR) {
        i_y++;
        // std::cout << "errory" << error_y << std::endl;
        fix_violating_children(widget, 1, error_y, num_children);
        all_children_y = _get_total_child_size(widget, 1);
        error_y = all_children_y - my_size_y;
        if (i_y > 100) break;
    }

    // Solve for children
    for (Widget* child : widget->children) {
        solve_violations(child);
    }
}

void compute_relative_positions(Widget* widget) {
    if (widget->parent == nullptr) {
        // This already happens by default, but lets be explicit about it
        widget->computed_relative_pos[0] = 0.f;
        widget->computed_relative_pos[1] = 0.f;
    }

    // Assuming we dont care about things smaller than 1 pixel
    widget->computed_size[0] = round(widget->computed_size[0]);
    widget->computed_size[1] = round(widget->computed_size[1]);
    float sx = widget->computed_size[0];
    float sy = widget->computed_size[1];

    float offx = 0.f;
    float offy = 0.f;

    // Represents the current wrap's largest
    // ex. on Column mode we only care about where to start the next column
    float col_w = 0.f;
    float col_h = 0.f;

    const auto update_max_size = [&](float cx, float cy) {
        col_w = fmax(cx, col_w);
        col_h = fmax(cy, col_h);
    };

    for (Widget* child : widget->children) {
        float cx = child->computed_size[0];
        float cy = child->computed_size[1];

        bool will_hit_max_x = false;
        bool will_hit_max_y = false;

        will_hit_max_x = cx + offx > sx;
        will_hit_max_y = cy + offy > sy;

        // We cant grow and are going over the limit
        if (widget->growflags & GrowFlags::None &&
            (will_hit_max_x || will_hit_max_y)) {
            child->computed_relative_pos[0] = sx;
            child->computed_relative_pos[1] = sy;
            continue;
        }

        // We can grow vertically and current child will push us over height
        // lets wrap
        if (widget->growflags & GrowFlags::Column && cy + offy > sy) {
            offy = 0;
            offx += col_w;
            col_w = cx;
        }

        // We can grow horizontally and current child will push us over
        // width lets wrap
        if (widget->growflags & GrowFlags::Row && cx + offx > sx) {
            offx = 0;
            offy += col_h;
            col_h = cy;
        }

        child->computed_relative_pos[0] = offx;
        child->computed_relative_pos[1] = offy;

        // Setup for next child placement
        if (widget->growflags & GrowFlags::Column) {
            offy += cy;
        }
        if (widget->growflags & GrowFlags::Row) {
            offx += cx;
        }

        update_max_size(cx, cy);
        compute_relative_positions(child);
    }
}

void compute_rect_bounds(Widget* widget) {
    // std::cout << "computing rect bounds for " << widget << std::endl;
    vec2 offset = vec2{0.f, 0.f};
    Widget* parent = widget->parent;
    if (parent) {
        offset = offset + vec2{parent->rect.x, parent->rect.y};
        // parent = parent->parent;
    }

    Rectangle rect;
    rect.x = offset.x + widget->computed_relative_pos[0];
    rect.y = offset.y + widget->computed_relative_pos[1];
    rect.width = widget->computed_size[0];
    rect.height = widget->computed_size[1];

    widget->rect = rect;

    for (Widget* child : widget->children) {
        compute_rect_bounds(child);
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
    // - (pre) compute rect bounds
    compute_rect_bounds(widget);
}
}  // namespace autolayout
}  // namespace ui
