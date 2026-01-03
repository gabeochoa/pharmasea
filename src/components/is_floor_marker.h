
#pragma once

#include "../engine/log.h"
#include "base_component.h"

constexpr int MAX_FLOOR_MARKERS = 100;

struct IsFloorMarker : public BaseComponent {
    enum Type {
        Unset,
        Planning_SpawnArea,
        Planning_TrashArea,
        Store_SpawnArea,
        Store_PurchaseArea,
        Store_LockedArea,
    } type = Unset;

    explicit IsFloorMarker(Type type) : type(type) {}
    IsFloorMarker() : IsFloorMarker(Unset) {}

    [[nodiscard]] bool is_marked(int entity_id) const {
        return std::find(marked_entities.begin(), marked_entities.end(),
                         entity_id) != marked_entities.end();
    }

    [[nodiscard]] const auto& marked_ids() const { return marked_entities; }

    void mark(int entity_id) {
        if (marked_entities.size() >= MAX_FLOOR_MARKERS) {
            log_warn("can only mark {} items and already marked that many",
                     MAX_FLOOR_MARKERS);
            return;
        }
        marked_entities.push_back(entity_id);
    }

    void mark_all(std::vector<int>&& ids) { marked_entities = ids; }

    void clear() { marked_entities.clear(); }

    [[nodiscard]] size_t num_marked() const { return marked_entities.size(); }
    [[nodiscard]] bool has_any_marked() const {
        return !marked_entities.empty();
    }

   private:
    std::vector<int> marked_entities;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.type,                       //
            self.marked_entities             //
        );
    }
};
