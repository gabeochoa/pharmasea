
#pragma once

#include "base_component.h"

struct IsToilet : public BaseComponent {
    virtual ~IsToilet() {}

    enum State { Available, InUse, NeedsCleaning } state = Available;
    int total_uses = 3;
    int uses_remaining = 3;
    //
    int user = -1;

    [[nodiscard]] bool occupied() const { return state != Available; }
    [[nodiscard]] bool available() const { return !occupied(); }
    [[nodiscard]] bool is_user(int id) const { return user == id; }

    void start_use(int id) {
        user = id;
        state = InUse;
    }

    void end_use() {
        user = -1;
        uses_remaining--;
        state = uses_remaining <= 0 ? NeedsCleaning : Available;
    }

    void reset() {
        uses_remaining = total_uses;
        state = Available;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(state);
        s.value4b(total_uses);
        s.value4b(uses_remaining);
    }
};
