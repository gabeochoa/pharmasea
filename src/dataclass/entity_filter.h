#pragma once

#include <exception>
#include <optional>
//

#include "../engine/type_name.h"
#include "../entity.h"
//
#include "../components/has_subtype.h"
#include "../components/is_drink.h"
#include "../components/is_item.h"
#include "ingredient.h"

enum RespectFilter { All, ReqOnly, Ignore };

struct EntityFilter {
    enum FilterStrength { Suggestion, Requirement } strength;

    enum FilterDatumType {
        EmptyFilterDatumType = 0,
        Name = 1 << 0,
        Subtype = 1 << 1,
        Ingredients = 1 << 2,
    } flags;

   private:
    std::optional<EntityType> entity_type;
    int subtype_index;
    std::optional<IngredientBitSet> ingredients;

   public:
    [[nodiscard]] size_t type_count() const {
        return magic_enum::enum_count<FilterDatumType>();
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

    [[nodiscard]] std::string print_value_for_type(FilterDatumType type) const {
        switch (type) {
            case Name:
                return fmt::format("{}", entity_type.has_value()
                                             ? str(entity_type.value())
                                             : "no entity_type");
            case Subtype:
                return fmt::format("{}", subtype_index);
            case Ingredients:
                return ingredients.has_value()
                           ? fmt::format("{}", ingredients.value())
                           : fmt::format("{}", "no ingredient filter");
            case EmptyFilterDatumType:
                break;
        }
        return "EntityFilter::UNSET";
    }

    void clear() {
        const auto clear_type = [this](FilterDatumType type) {
            switch (type) {
                case Name:
                    entity_type = {};
                    break;
                case Subtype:
                    subtype_index = -1;
                    break;
                case Ingredients:
                    ingredients = {};
                    break;
                case EmptyFilterDatumType:
                    break;
            }
        };

        magic_enum::enum_for_each<FilterDatumType>([clear_type](auto val) {
            constexpr FilterDatumType type = val;
            clear_type(type);
        });
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
                return entity_type.has_value();
            case Subtype:
                return subtype_index != -1;
            case Ingredients:
                return ingredients.has_value();
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
        if constexpr (std::is_same_v<T, EntityType>) {
            if (type & FilterDatumType::Name) this->entity_type = (value);
        } else if constexpr (std::is_same_v<T, int>) {
            if (type & FilterDatumType::Subtype) subtype_index = (value);
        } else if constexpr (std::is_same_v<T, IngredientBitSet>) {
            if (type & FilterDatumType::Ingredients) ingredients = (value);
        }
        return *this;
    }

    template<typename T>
    T read_filter_value_from_entity(const Entity& entity,
                                    FilterDatumType type) const {
        if constexpr (std::is_same_v<T, std::string>) {
            if (type & FilterDatumType::Name) return std::string(entity.name());
        } else if constexpr (std::is_same_v<T, EntityType>) {
            if (type & FilterDatumType::Name) {
                return entity.type;
            }
        } else if constexpr (std::is_same_v<T, int>) {
            if (type & FilterDatumType::Subtype) {
                if (entity.is_missing<HasSubtype>()) return -1;
                return entity.get<HasSubtype>().get_type_index();
            }
        } else if constexpr (std::is_same_v<T, IngredientBitSet>) {
            if (type & FilterDatumType::Ingredients) {
                if (entity.is_missing<IsDrink>()) return {};
                return entity.get<IsDrink>().ing();
            }
        }
        log_warn("EntityFilter:: reading value from entity but no match for {}",
                 type_name<T>());
        throw std::runtime_error("Reading Value from Entity but no match");
    }

    EntityFilter& set_filter_with_entity(const Entity& data) {
        // stores the information from data in that flag

        if (flags & FilterDatumType::Name)
            set_filter_value_for_type<EntityType>(
                FilterDatumType::Name,
                read_filter_value_from_entity<EntityType>(
                    data, FilterDatumType::Name));

        if (flags & FilterDatumType::Subtype)
            set_filter_value_for_type<int>(FilterDatumType::Subtype,
                                           read_filter_value_from_entity<int>(
                                               data, FilterDatumType::Subtype));

        if (flags & FilterDatumType::Ingredients)
            set_filter_value_for_type<IngredientBitSet>(
                FilterDatumType::Ingredients,
                read_filter_value_from_entity<IngredientBitSet>(
                    data, FilterDatumType::Ingredients));

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
                return check_type(entity, entity_type.value());
            case Subtype:
                return read_filter_value_from_entity<int>(entity, type) ==
                       subtype_index;
                break;
            case Ingredients:
                return read_filter_value_from_entity<IngredientBitSet>(
                           entity, type) == ingredients.value();
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
        s.ext(entity_type, bitsery::ext::StdOptional{},
              [](S& sv, EntityType& val) { sv.value4b(val); });
        s.value4b(subtype_index);
    }
};
