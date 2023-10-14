
#pragma once

#include "../engine/log.h"
#include "base_component.h"

const int MAX_FLOOR_MARKERS = 100;

struct IsFloorMarker : public BaseComponent {
    enum Type {
        Unset,
        Planning_SpawnArea,
        Planning_TrashArea,
        Store_SpawnArea,
        Store_PurchaseArea,
    } type = Unset;

    explicit IsFloorMarker(Type type) : type(type) {}
    IsFloorMarker() : IsFloorMarker(Unset) {}

    [[nodiscard]] bool is_marked(int entity_id) const {
        return std::find(marked_entities.begin(), marked_entities.end(),
                         entity_id) != marked_entities.end();
    }

    [[nodiscard]] const auto& marked_ids() const { return marked_entities; }

    void mark(int entity_id) {
        if (next_index >= MAX_FLOOR_MARKERS) {
            log_warn("can only mark {} items and already marked that many",
                     MAX_FLOOR_MARKERS);
            return;
        }
        marked_entities[next_index] = entity_id;
        next_index++;
    }

    void clear() { next_index = 0; }

    [[nodiscard]] int num_marked() const { return next_index + 1; }
    [[nodiscard]] bool has_any_marked() const { return next_index != 0; }

   private:
    std::array<int, MAX_FLOOR_MARKERS> marked_entities;
    int next_index = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(type);

        s.value4b(next_index);
        s.container4b(marked_entities);
    }
};
