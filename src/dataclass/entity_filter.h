#pragma once

#include <exception>
#include <optional>
//

#include "../entity.h"
//
#include "../components/debug_name.h"
#include "../components/has_subtype.h"
#include "../components/is_item.h"

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
        throw std::runtime_error("Reading Value from Entity but no match");
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
