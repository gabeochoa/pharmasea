
#pragma once

#include "../vec_util.h"
#include "base_component.h"

struct IsNux : public BaseComponent {
    bool is_active = false;
    std::function<bool()> shouldTrigger;
    std::function<bool()> isComplete;

    int entityID = -1;

    // TODO should we be able to serialize TranslatableContent?
    std::string content;
    static constexpr int MAX_CONTENT_LENGTH = 100;

    [[nodiscard]] bool is_attached() const { return entityID != -1; }

    auto& set_eligibility_fn(const std::function<bool()>& trigger) {
        shouldTrigger = trigger;
        return *this;
    }

    auto& set_completion_fn(const std::function<bool()>& trigger) {
        isComplete = trigger;
        return *this;
    }

    auto& should_attach_to(int id) {
        entityID = id;
        return *this;
    }

    auto& set_content(const std::string& c) {
        content = c;
        return *this;
    }

    virtual ~IsNux() {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(is_active);
        s.value4b(entityID);
        s.text1b(content, MAX_CONTENT_LENGTH);
    }
};

struct IsNuxManager : public BaseComponent {
    virtual ~IsNuxManager() {}

    bool initialized = false;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(initialized);
    }
};
