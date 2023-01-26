

#pragma once

#include "base_component.h"

struct Entity;

struct HasWork : public BaseComponent {
    HasWork() : pct_work_complete(0.f), more_to_do(false) {}
    virtual ~HasWork() {}

    // Does this have work to be done?
    [[nodiscard]] bool is_work_complete() const {
        return pct_work_complete >= 1.f;
    }
    [[nodiscard]] bool has_work() const { return more_to_do; }
    [[nodiscard]] bool doesnt_have_work() const { return !has_work(); }
    [[nodiscard]] bool can_show_progress_bar() const {
        return has_work() && pct_work_complete >= 0.01f;
    }
    [[nodiscard]] bool dont_show_progres_bar() const {
        return !can_show_progress_bar();
    }

    void increase_pct(float amt) { pct_work_complete += amt; }
    void update_pct(float pct) { pct_work_complete = pct; }
    void set_has_more_work() { more_to_do = true; }
    void reset_pct() { pct_work_complete = 0.f; }

    [[nodiscard]] int scale_length(int length) {
        return (int) (pct_work_complete * length);
    }

    // TODO make private
    std::function<void(HasWork&, std::shared_ptr<Entity> person, float dt)>
        do_work;

    void init(
        std::function<void(HasWork&, std::shared_ptr<Entity> person, float dt)>
            worker) {
        do_work = worker;
    }

   private:
    float pct_work_complete;
    bool more_to_do;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        //
        s.value1b(more_to_do);
        s.value4b(pct_work_complete);
    }
};
