
#pragma once

#include <map>

#include "../vendor_include.h"
#include "base_component.h"

enum struct AttributeFlags {
    None = 0,
    Dirty = 1 << 0,
    CleanShiny = 1 << 1,
};

struct CanAffectFloorAttributes : public BaseComponent {
    AttributeFlags removeOnCleanup;

    auto& set_on_cleanup_flag(AttributeFlags flags) {
        removeOnCleanup = flags;
        return *this;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(removeOnCleanup);
    }
};

struct IsFloorAttributeManager : public BaseComponent {
    const static int MAX_MAP_SIZE = 1000;
    virtual ~IsFloorAttributeManager() {}

    [[nodiscard]] AttributeFlags get_flags(const vec2& pos) const {
        if (attrs.contains(pos)) {
            return attrs.at(pos);
        }
        return AttributeFlags{};
    }

    void set_flags(const vec2& pos, AttributeFlags flags) {
        if (!attrs.contains(pos)) {
            attrs[pos] = AttributeFlags{};
        }
        auto existing = attrs[pos];
        existing |= flags;
        attrs[pos] = existing;
    }

    void remove_set_flag(const vec2& pos, AttributeFlags flag) {
        if (!attrs.contains(pos)) {
            return;
        }
        auto existing = attrs[pos];
        existing &= ~flag;
        attrs[pos] = existing;
    }

    void clear_all_flags(const vec2& pos) {
        if (!attrs.contains(pos)) return;
        attrs[pos] = AttributeFlags{};
    }

    void remove_empty_flags() {
        std::erase_if(attrs, [](const auto& item) {
            auto const& [key, value] = item;
            return value == AttributeFlags{};
        });
    }

    [[nodiscard]] const std::map<vec2, AttributeFlags>& all_flags() const {
        return attrs;
    }

   private:
    std::map<vec2, AttributeFlags> attrs;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(attrs, bitsery::ext::StdMap{MAX_MAP_SIZE},
              [](S& sv, vec2& key, AttributeFlags(&value)) {
                  sv.object(key);
                  sv.value4b(value);
              });
    }
};
