#pragma once

#include "base_component.h"

struct IsTriggerArea : public BaseComponent {
    virtual ~IsTriggerArea() {}

    [[nodiscard]] const std::string& title() const { return _title; }

    void update_title(const std::string& nt) {
        _title = nt;
        if ((int) _title.size() > max_title_length) {
            log_warn(
                "title for trigger area is too long, max is {} chars, you had "
                "{}",
                max_title_length, _title.size());
            _title.resize(max_title_length);
        }
    }

   private:
    std::string _title;
    int max_title_length = 20;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(max_title_length);
        s.text1b(_title, max_title_length);
    }
};
