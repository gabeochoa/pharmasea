

#pragma once

#include "base_component.h"

struct Entity;

struct HasWork : public BaseComponent {
    HasWork()
        :  // Start with 0% progress
          pct_work_complete(0.f),
          //  this forces the 'has more' to run by default
          more_to_do(true),
          // TODO what breaks if we turn this on?
          reset_on_empty(false),
          hide_progress_bar_on_full(false),
          hide_progress_bar(false) {}
    virtual ~HasWork() {}

    // Does this have work to be done?
    [[nodiscard]] bool is_work_complete() const {
        return pct_work_complete >= 1.f;
    }
    [[nodiscard]] bool has_work() const { return more_to_do; }
    [[nodiscard]] bool doesnt_have_work() const { return !has_work(); }
    [[nodiscard]] bool can_show_progress_bar() const {
        if (hide_progress_bar_on_full && is_work_complete()) return false;
        return has_work() && pct_work_complete >= 0.01f;
    }
    [[nodiscard]] bool dont_show_progress_bar() const {
        return !can_show_progress_bar();
    }

    void increase_pct(float amt) { pct_work_complete += amt; }
    void update_pct(float pct) { pct_work_complete = pct; }
    void set_has_more_work() { more_to_do = true; }
    void reset_pct() { pct_work_complete = 0.f; }

    [[nodiscard]] int scale_length(int length) const {
        return static_cast<int>(pct_work_complete * length);
    }

    [[nodiscard]] float scale_length(float length) const {
        return pct_work_complete * length;
    }

    using WorkFn = std::function<void(Entity&, HasWork&, Entity&, float)>;

    auto& init(const WorkFn& worker) {
        do_work = worker;
        return *this;
    }

    [[nodiscard]] bool should_reset_on_empty() const { return reset_on_empty; }
    void set_reset_on_empty(bool roe) { reset_on_empty = roe; }

    void call(Entity& owner, Entity& player, float dt) {
        if (do_work) do_work(owner, *this, player, dt);
    }

    void call(HasWork& other, Entity& owner, Entity& player, float dt) {
        if (do_work) do_work(owner, other, player, dt);
    }

    auto& set_hide_on_full(bool hof) {
        hide_progress_bar_on_full = hof;
        return *this;
    }

    auto& set_hide_always(bool hof) {
        hide_progress_bar = hof;
        return *this;
    }

   private:
    WorkFn do_work;

    float pct_work_complete;
    bool more_to_do;
    bool reset_on_empty;
    bool hide_progress_bar_on_full;
    bool hide_progress_bar;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                hide_progress_bar_on_full, pct_work_complete);
    }
};

CEREAL_REGISTER_TYPE(HasWork);
