
#pragma once

#include <map>

#include "../vec_util.h"
#include "../vendor_include.h"
#include "base_component.h"

enum struct AttributeFlags {
    None = 0,
    Dirty = 1 << 0,
    CleanShiny = 1 << 1,
    CleanShiny2 = 1 << 2,
    CleanShiny3 = 1 << 3,
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

    struct Snap {
        vec2 pos;
        Snap() {}
        explicit Snap(vec2 p) : pos(vec::snap(p)) {}

        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.object(pos);
        }

        bool operator<(const Snap& other) const { return pos < other.pos; }
    };

    [[nodiscard]] AttributeFlags get_flags(const vec2& pos) const {
        if (attrs.contains(Snap(pos))) {
            return attrs.at(Snap(pos));
        }
        return AttributeFlags{};
    }

    [[nodiscard]] AttributeFlags set_(AttributeFlags flag,
                                      AttributeFlags toset) {
        flag |= toset;
        return flag;
    }

    [[nodiscard]] AttributeFlags unset_(AttributeFlags flag,
                                        AttributeFlags toset) {
        flag &= ~toset;
        return flag;
    }

    void set_flags(const vec2& pos, AttributeFlags flags) {
        if (!attrs.contains(Snap(pos))) {
            attrs[Snap(pos)] = AttributeFlags{};
        }
        auto existing = attrs[Snap(pos)];
        attrs[Snap(pos)] = set_(existing, flags);
    }

    void remove_set_flag(const vec2& pos, AttributeFlags flag) {
        if (!attrs.contains(Snap(pos))) {
            return;
        }
        auto existing = attrs[Snap(pos)];
        attrs[Snap(pos)] = unset_(existing, flag);
    }

    void clear_all_flags(const vec2& pos) {
        if (!attrs.contains(Snap(pos))) return;
        attrs[Snap(pos)] = AttributeFlags{};
    }

    void remove_empty_flags() {
        std::erase_if(attrs, [](const auto& item) {
            auto const& [key, value] = item;
            return value == AttributeFlags{};
        });
    }

    [[nodiscard]] bool is_spot_too_clean(const vec2& pos) const {
        AttributeFlags flag = get_flags(pos);
        return is_too_clean(flag);
    }

    [[nodiscard]] bool is_too_clean(const AttributeFlags& flag) const {
        AttributeFlags clean_flags = AttributeFlags::CleanShiny |
                                     AttributeFlags::CleanShiny2 |
                                     AttributeFlags::CleanShiny3;
        return (flag & clean_flags) != AttributeFlags::None;
    }

    bool make_spot_dirtier(const vec2& pos) {
        attrs[Snap(pos)] = mutate_clean(get_flags(pos));
        return is_spot_too_clean(pos);
    }

    [[nodiscard]] AttributeFlags mutate_clean(AttributeFlags flag) {
        switch (flag) {
            case AttributeFlags::None:
            case AttributeFlags::Dirty:
                break;
            case AttributeFlags::CleanShiny:
                flag = unset_(flag, AttributeFlags::CleanShiny);
                break;
            case AttributeFlags::CleanShiny2:
                flag = unset_(flag, AttributeFlags::CleanShiny2);
                flag = set_(flag, AttributeFlags::CleanShiny);
                break;
            case AttributeFlags::CleanShiny3:
                flag = unset_(flag, AttributeFlags::CleanShiny3);
                flag = set_(flag, AttributeFlags::CleanShiny2);
                break;
        }
        return flag;
    }

    [[nodiscard]] const std::map<Snap, AttributeFlags>& all_flags() const {
        return attrs;
    }

   private:
    std::map<Snap, AttributeFlags> attrs;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.ext(attrs, bitsery::ext::StdMap{MAX_MAP_SIZE},
              [](S& sv, Snap& key, AttributeFlags(&value)) {
                  sv.object(key);
                  sv.value4b(value);
              });
    }
};
