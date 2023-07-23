#pragma once

#include <optional>
//

#include "../entity.h"
#include "base_component.h"
//
#include "debug_name.h"
#include "has_subtype.h"
#include "is_item.h"

enum RespectFilter { All, ReqOnly, Ignore };

struct EntityFilter {
    enum FilterStrength { Suggestion, Requirement } strength;

    enum FilterDatumType {
        EmptyFilterDatumType = 0,
        Name = 1 << 0,
        Subtype = 1 << 1,
    } flags;

    [[nodiscard]] size_t type_count() const {
        return magic_enum::enum_count<FilterDatumType>();
    }

    std::optional<std::string> name;
    int subtype_index;

    [[nodiscard]] std::string print_value_for_type(FilterDatumType type) const {
        switch (type) {
            case Name:
                return fmt::format("{}",
                                   name.has_value() ? name.value() : "no name");
            case Subtype:
                return fmt::format("{}", subtype_index);
            case EmptyFilterDatumType:
                break;
        }
        return "EntityFilter::UNSET";
    }

    [[nodiscard]] bool any_flags() const {
        constexpr auto types = magic_enum::enum_values<FilterDatumType>();
        std::size_t i = 0;
        while (i < type_count()) {
            FilterDatumType filter_type = types[i];
            if (filter_enabled(filter_type)) {
                return true;
            }
            i++;
        }
        return false;
    }

    [[nodiscard]] bool no_flags() const { return !any_flags(); }

    void clear() {
        magic_enum::enum_for_each<FilterDatumType>([this](auto val) {
            constexpr FilterDatumType type = val;
            clear_type(type);
        });
    }

    void clear_type(FilterDatumType type) {
        switch (type) {
            case Name:
                name = {};
                break;
            case Subtype:
                subtype_index = -1;
                break;
            case EmptyFilterDatumType:
                break;
        }
    }

    // Is this type of filtering enabled on our object?
    [[nodiscard]] bool filter_enabled(FilterDatumType type) const {
        return (flags & type);
    }

    [[nodiscard]] bool filter_enabled_and_set(FilterDatumType type) const {
        return filter_enabled(type) && filter_has_value(type);
    }

    // Does the filter have a value set for the filter type?
    [[nodiscard]] bool filter_has_value(FilterDatumType type) const {
        switch (type) {
            case Name:
                return name.has_value();
            case Subtype:
                return subtype_index != -1;
            case EmptyFilterDatumType:
                break;
        }
        return false;
    }

    // For all the filter types we have enabled,
    // are all of them set?
    [[nodiscard]] bool filter_is_set() const {
        constexpr auto types = magic_enum::enum_values<FilterDatumType>();

        bool all_set = true;
        std::size_t i = 0;
        while (i < type_count()) {
            FilterDatumType filter_type = types[i];
            if (filter_enabled(filter_type)) {
                all_set = all_set && filter_has_value(filter_type);
            }
            i++;
        }
        return all_set;
    }

    template<typename T>
    EntityFilter& set_filter_value_for_type(FilterDatumType type,
                                            const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            if (type & FilterDatumType::Name) this->name = (value);
        } else if constexpr (std::is_same_v<T, int>) {
            if (type & FilterDatumType::Subtype) subtype_index = (value);
        }
        return *this;
    }

    template<typename T>
    T read_filter_value_from_entity(const Entity& entity,
                                    FilterDatumType type) const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (type & FilterDatumType::Name)
                return entity.get<DebugName>().name();
        } else if constexpr (std::is_same_v<T, int>) {
            if (type & FilterDatumType::Subtype) {
                if (entity.is_missing<HasSubtype>()) return -1;
                return entity.get<HasSubtype>().get_type_index();
            }
        }
        log_warn("EntityFilter:: reading value from entity but no match");
        return 0;
    }

    EntityFilter& set_filter_with_entity(const Entity& data) {
        // stores the information from data in that flag

        if (flags & FilterDatumType::Name)
            set_filter_value_for_type<std::string>(
                FilterDatumType::Name,
                read_filter_value_from_entity<std::string>(
                    data, FilterDatumType::Name));

        if (flags & FilterDatumType::Subtype)
            set_filter_value_for_type<int>(FilterDatumType::Subtype,
                                           read_filter_value_from_entity<int>(
                                               data, FilterDatumType::Subtype));

        return *this;
    }

    EntityFilter& set_enabled_flags(FilterDatumType fs) {
        flags = fs;
        return *this;
    }

    EntityFilter& set_filter_strength(FilterStrength fs) {
        strength = fs;
        return *this;
    }

    [[nodiscard]] bool match_specific_filter(FilterDatumType type,
                                             const Entity& entity) const {
        switch (type) {
            case Name:
                return check_name(entity, name.value().c_str());
            case Subtype:
                return read_filter_value_from_entity<int>(entity, type) ==
                       subtype_index;
                break;
            case EmptyFilterDatumType:
                break;
        }
        log_warn("match specific filter; fell through case: {}", type);
        return false;
    }

    [[nodiscard]] bool matches(const Entity& entity,
                               RespectFilter respect) const {
        if (respect == Ignore) return true;
        if (respect == ReqOnly && strength == Suggestion) return true;

        bool pass = true;

        auto types = magic_enum::enum_values<FilterDatumType>();

        size_t i = 0;
        while (i < type_count()) {
            auto type = types[i];
            if (filter_enabled_and_set(type)) {
                pass = pass && match_specific_filter(type, entity);
            }
            i++;
        }
        //
        return pass;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(flags);
        s.ext(name, bitsery::ext::StdOptional{},
              [](S& sv, std::string& val) { sv.text1b(val, 25); });
        s.value4b(subtype_index);
    }
};

struct CanHoldItem : public BaseComponent {
    CanHoldItem() : held_by(IsItem::HeldBy::UNKNOWN) {}
    CanHoldItem(IsItem::HeldBy hb) : held_by(hb) {}

    virtual ~CanHoldItem() {}

    [[nodiscard]] bool empty() const { return held_item == nullptr; }
    // Whether or not this entity has something we can take from them
    [[nodiscard]] bool is_holding_item() const { return !empty(); }

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> asT() const {
        return dynamic_pointer_cast<T>(held_item);
    }

    CanHoldItem& update(std::shared_ptr<Entity> item) {
        if (held_item != nullptr && !held_item->cleanup &&
            //
            (held_item->has<IsItem>() && !held_item->get<IsItem>().is_held())
            //
        ) {
            log_warn(
                "you are updating the held item to null, but the old one isnt "
                "marked cleanup (and not being held) , this might be an issue "
                "if you are tring to "
                "delete it");
        }

        held_item = item;
        if (held_item) held_item->get<IsItem>().set_held_by(held_by);
        if (held_item && held_by == IsItem::HeldBy::UNKNOWN) {
            log_warn(
                "We never had our HeldBy set, so we are holding {}{}  by "
                "UNKNOWN",
                item->id, item->get<DebugName>().name());
        }
        return *this;
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

    CanHoldItem& set_filter(EntityFilter ef) {
        filter = ef;
        return *this;
    }

    [[nodiscard]] bool can_hold(const Entity& item,
                                RespectFilter respect_filter) const {
        if (item.has<IsItem>()) {
            bool cbhb = item.get<IsItem>().can_be_held_by(held_by);
            // log_info("trying to pick up {} with {} and {} ",
            // item.get<DebugName>(), held_by, cbhb);
            if (!cbhb) return false;
        }
        if (!filter.matches(item, respect_filter)) return false;
        // By default accept anything
        return true;
    }

    CanHoldItem& update_held_by(IsItem::HeldBy hb) {
        held_by = hb;
        return *this;
    }

    [[nodiscard]] EntityFilter& get_filter() { return filter; }
    [[nodiscard]] const EntityFilter& get_filter() const { return filter; }

   private:
    std::shared_ptr<Entity> held_item = nullptr;
    EntityFilter filter;
    IsItem::HeldBy held_by;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(held_item, bitsery::ext::StdSmartPtr{});

        // TODO we only need this for debug info
        s.object(filter);
    }
};
