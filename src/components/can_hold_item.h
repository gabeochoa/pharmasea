#pragma once

#include "../entity.h"
#include "base_component.h"
//
#include "is_item.h"

struct CanHoldItem : public BaseComponent {
    virtual ~CanHoldItem() {}

    typedef std::function<bool(const Entity&)> Filterfn;

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_item);
    }

    void update(std::shared_ptr<Entity> item,
                IsItem::HeldBy newHB = IsItem::HeldBy::NONE) {
        if (held_item != nullptr && !held_item->cleanup) {
            log_warn(
                "you are updating the held item to null, but the old one isnt "
                "marked cleanup, this might be an issue if you are tring to "
                "delete it");
        }

        held_item = item;
        if (held_item) held_item->get<IsItem>().set_held_by(newHB);
    }

    // TODO this isnt const because we want to write to the item
    // we could make this const and then expose certain things that we want to
    // change separately like 'held_by'
    // (change to use update instead and make this const)
    [[nodiscard]] std::shared_ptr<Entity>& item() { return held_item; }

    // const?
    [[nodiscard]] const std::shared_ptr<Entity> const_item() const {
        return held_item;
    }

    void set_filter_fn(Filterfn fn = nullptr) { filter = fn; }

    [[nodiscard]] bool can_hold(const Entity& item) const {
        if (filter) return filter(item);
        // By default accept anything
        return true;
    }

   private:
    std::shared_ptr<Entity> held_item = nullptr;
    Filterfn filter;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(held_item, bitsery::ext::StdSmartPtr{});
    }
};
