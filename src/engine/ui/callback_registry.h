
#pragma once

#include <functional>
#include <vector>

#include "ui_context.h"

namespace ui {

class CallbackRegistry {
   public:
    void register_call(std::shared_ptr<ui::UIContext> ui_context,
                       const std::function<void(void)>& cb, int z_index = 0) {
        if (ui_context->should_immediate_draw) {
            cb();
            return;
        }
        callbacks.emplace_back(ScheduledCall{z_index, cb});
        insert_sorted();
    }

    void execute_callbacks() {
        for (const auto& entry : callbacks) {
            entry.callback();
        }
        callbacks.clear();
    }

   private:
    struct ScheduledCall {
        int z_index;
        std::function<void(void)> callback;
    };

    std::vector<ScheduledCall> callbacks;

    void insert_sorted() {
        if (callbacks.size() <= 1) {
            return;
        }

        auto it = callbacks.end() - 1;
        while (it != callbacks.begin() && (it - 1)->z_index < it->z_index) {
            std::iter_swap(it - 1, it);
            --it;
        }
    }
};

}  // namespace ui
