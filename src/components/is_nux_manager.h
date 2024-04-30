
#pragma once

#include "../entity_type.h"
#include "../strings.h"
#include "../vec_util.h"
#include "base_component.h"

struct IsNux;

struct IsNux : public BaseComponent {
    bool is_active = false;
    std::function<bool(const IsNux&)> shouldTrigger;
    std::function<bool(const IsNux&)> isComplete;

    int entityID = -1;
    EntityType ghost = EntityType::Unknown;

    TranslatableString content;

    [[nodiscard]] bool is_attached() const { return entityID != -1; }

    auto& set_eligibility_fn(const std::function<bool(const IsNux&)>& trigger) {
        shouldTrigger = trigger;
        return *this;
    }

    auto& set_completion_fn(const std::function<bool(const IsNux&)>& trigger) {
        isComplete = trigger;
        return *this;
    }

    auto& should_attach_to(int id) {
        entityID = id;
        return *this;
    }

    auto& set_content(const TranslatableString& c) {
        content = c;
        return *this;
    }

    auto& set_ghost(const EntityType& g) {
        ghost = g;
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
        s.object(content);
        s.value4b(ghost);
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
