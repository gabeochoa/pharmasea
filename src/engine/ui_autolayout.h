
#pragma once

#include "../vec_util.h"
#include "assert.h"
#include "log.h"
#include "ui_widget.h"

namespace ui {
namespace autolayout {
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

const float ACCEPTABLE_ERROR = 0.5f;

float compute_size_for_standalone_expectation(Widget* widget, int exp_index) {
    log_trace("csfse {} {}", *widget, exp_index);
    SizeExpectation exp = widget->size_expected[exp_index];
    switch (exp.mode) {
        case SizeMode::Pixels:
            return exp.value;
        case SizeMode::Text:
            // TODO figure this out
            // So we can use MeasureTextEx but
            // we need to know the font and spacing
            return 100.f;
        default:
            return widget->computed_size[exp_index];
    }
}

float compute_size_for_parent_expectation(Widget* widget, int exp_index) {
    if (widget->absolute &&
        widget->size_expected[exp_index].mode == SizeMode::Percent) {
        widget->print_tree();
        VALIDATE(false, "Absolute widgets should not use Percent");
    }

    float no_change = widget->computed_size[exp_index];
    if (!widget->parent || widget->absolute) return no_change;
    float parent_size = widget->parent->computed_size[exp_index];
    SizeExpectation exp = widget->size_expected[exp_index];
    float new_size = exp.value * parent_size;
    switch (exp.mode) {
        case SizeMode::Percent:
            return parent_size == -1 ? no_change : new_size;
        default:
            return no_change;
    }
}

float _sum_children_axis_for_child_exp(Widget* widget, int exp_index) {
    float total_child_size = 0.f;
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        float cs = child->computed_size[exp_index];
        if (cs == -1) {
            if (child->size_expected[exp_index].mode == SizeMode::Percent &&
                widget->size_expected[exp_index].mode == SizeMode::Children) {
                VALIDATE(false,
                         "Parents sized with mode 'children' cannot have "
                         "children sized with mode 'percent'.");
            }
            widget->print_tree();
            // Instead of silently ignoring this, check the cases above
            VALIDATE(false, "expect that all children have been solved by now");
        }
        total_child_size += cs;
    }
    return total_child_size;
}

float _max_child_size(Widget* widget, int exp_index) {
    float max_child_size = 0.f;
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        float cs = child->computed_size[exp_index];
        if (cs == -1) {
            if (child->size_expected[exp_index].mode == SizeMode::Percent &&
                widget->size_expected[exp_index].mode == SizeMode::Children) {
                VALIDATE(false,
                         "Parents sized with mode 'children' cannot have "
                         "children sized with mode 'percent'.");
            }
            // Instead of silently ignoring this, check the cases above
            VALIDATE(false, "expect that all children have been solved by now");
        }
        max_child_size = fmaxf(max_child_size, cs);
    }
    return max_child_size;
}

float compute_size_for_child_expectation(Widget* widget, int exp_index) {
    log_trace("csfce {} {}", *widget, exp_index);
    float no_change = widget->computed_size[exp_index];
    if (widget->children.empty()) return no_change;

    float total_child_size =
        _sum_children_axis_for_child_exp(widget, exp_index);
    float max_child_size = _max_child_size(widget, exp_index);

    float expectation = total_child_size;

    if (widget->growflags & GrowFlags::Column && exp_index == 0) {
        expectation = max_child_size;
    }

    SizeExpectation exp = widget->size_expected[exp_index];
    switch (exp.mode) {
        case SizeMode::Children:
            return expectation;
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
        // NOTE: specifically not ignoring absolute
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

float _get_total_child_size_for_violations(Widget* widget, int exp_index) {
    float sum = 0.f;
    if (exp_index == 0 && (widget->growflags & GrowFlags::Column)) {
        return widget->computed_size[0];
    }
    if (exp_index == 1 && (widget->growflags & GrowFlags::Row)) {
        return widget->computed_size[1];
    }
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        sum += child->computed_size[exp_index];
    }
    return sum;
}

void _solve_error_with_optional(Widget* widget, int exp_index, float* error) {
    int num_optional_children = 0;
    for (Widget* child : widget->children) {
        // Dont worry about absolute positioned children
        if (child->absolute) continue;

        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 0.f) {
            num_optional_children++;
        }
    }
    if (num_optional_children != 0) {
        float approx_epc = *error / num_optional_children;
        for (Widget* child : widget->children) {
            // Dont worry about absolute positioned children
            if (child->absolute) continue;

            SizeExpectation exp = child->size_expected[exp_index];
            if (exp.strictness == 0.f) {
                float cur_size = child->computed_size[exp_index];
                child->computed_size[exp_index] =
                    fmaxf(0, cur_size - approx_epc);
                if (cur_size > approx_epc) *error -= approx_epc;
            }
        }
    }
}

void fix_violating_children(Widget* widget, int exp_index, float error,
                            int num_children) {
    VALIDATE(num_children != 0, "Should never have zero children");

    int num_strict_children = 0;
    int num_ignorable_children = 0;
    for (Widget* child : widget->children) {
        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 1.f) {
            num_strict_children++;
        }
        if (child->absolute) {
            num_ignorable_children++;
        }
    }

    // If there is any error left,
    // we have to take away from the allowed children;

    int num_resizeable_children =
        num_children - num_strict_children - num_ignorable_children;

    // TODO we cannot enforce the assert below in the case of wrapping
    // because the positioning happens after error correction
    if (error > ACCEPTABLE_ERROR && num_resizeable_children == 0 &&
        //
        !((widget->growflags & GrowFlags::Column) ||
          (widget->growflags & GrowFlags::Row))
        //
    ) {
        widget->print_tree();
        log_warn("Error was {}", error);
        VALIDATE(
            num_resizeable_children > 0,
            "Cannot fit all children inside parent and unable to resize any of "
            "the children");
    }

    float approx_epc = error / (1.f * std::max(1, num_resizeable_children));
    for (Widget* child : widget->children) {
        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 1.f || child->absolute) {
            continue;
        }
        float portion_of_error = (1.f - exp.strictness) * approx_epc;
        float cur_size = child->computed_size[exp_index];
        child->computed_size[exp_index] = fmaxf(0, cur_size - portion_of_error);
        // Reduce strictness every round
        // so that eventually we will get there
        exp.strictness = fmaxf(0.f, exp.strictness - 0.05f);
        child->size_expected[exp_index] = exp;
    }
}

void tax_refund(Widget* widget, int exp_index, float error) {
    int num_eligible_children = 0;
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 0.f) {
            num_eligible_children++;
        }
    }

    if (num_eligible_children == 0) {
        log_trace("I have all this money to return, but no one wants it :(");
        return;
    }

    float indiv_refund = error / num_eligible_children;
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        SizeExpectation exp = child->size_expected[exp_index];
        if (exp.strictness == 0.f) {
            child->computed_size[exp_index] += abs(indiv_refund);
            log_trace("Just gave back, time for trickle down");
            tax_refund(child, exp_index, indiv_refund);
        }
        // TODO idk if we should do this for all non 1.f children?
        // if (exp.strictness == 1.f || child->ignore_size) {
        // continue;
        // }
    }
}

int _get_total_number_children(Widget* widget) {
    if (widget == nullptr) return 0;

    int i = 0;
    for (Widget* child : widget->children) {
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) continue;

        i++;
    }
    return i;
}

void solve_violations(Widget* widget) {
    int num_children = _get_total_number_children(widget);
    if (num_children == 0) return;

    // me -> left -> right

    float my_size_x = widget->computed_size[0];
    float my_size_y = widget->computed_size[1];

    float all_children_x = _get_total_child_size_for_violations(widget, 0);
    float error_x = all_children_x - my_size_x;
    int i_x = 0;
    log_trace("preopt errorx {} ", error_x);
    while (error_x > ACCEPTABLE_ERROR) {
        _solve_error_with_optional(widget, 0, &error_x);
        i_x++;
        log_trace("error x {}", error_x);
        fix_violating_children(widget, 0, error_x, num_children);
        all_children_x = _get_total_child_size_for_violations(widget, 0);
        error_x = all_children_x - my_size_x;
        if (i_x > 100) {
            log_warn("Hit X-axis iteration limit trying to solve violations");
            // VALIDATE(false, "hit x iteration limit trying to solve
            // violations");
            break;
        }
    }

    if (error_x < 0) {
        tax_refund(widget, 0, error_x);
    }

    float all_children_y = _get_total_child_size_for_violations(widget, 1);
    float error_y = all_children_y - my_size_y;
    int i_y = 0;
    log_trace("preopt errory {} ", error_y);
    while (error_y > ACCEPTABLE_ERROR) {
        _solve_error_with_optional(widget, 1, &error_y);
        i_y++;
        log_trace("error y {}", error_y);
        fix_violating_children(widget, 1, error_y, num_children);
        all_children_y = _get_total_child_size_for_violations(widget, 1);
        error_y = all_children_y - my_size_y;
        if (i_y > 100) {
            log_warn("Hit Y-axis iteration limit trying to solve violations");
            // VALIDATE(false, "hit y iteration limit trying to solve
            // violations");
            break;
        }
    }

    if (error_y < 0) {
        tax_refund(widget, 1, error_y);
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
        // Dont worry about any children that are absolutely positioned
        if (child->absolute) {
            compute_relative_positions(child);
            continue;
        }

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
    log_trace("computing rect bounds for {}", *widget);
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

// https://www.rfleury.com/p/ui-part-2-build-it-every-frame-immediate
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
