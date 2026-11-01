#pragma once

#include <optional>

#include "../components/can_hold_item.h"
#include "../components/indexer.h"
#include "../components/is_item_container.h"
#include "entity.h"
#include "entity_type.h"

namespace furniture {
// Forward declaration - defined in entity_makers.cpp
void make_itemcontainer(Entity& container, const DebugOptions& options,
                        vec2 pos, EntityType item_type);
}  // namespace furniture

class ItemContainerBuilder {
   public:
    ItemContainerBuilder(Entity& entity, vec2 pos, EntityType container_type)
        : m_entity(entity), m_pos(pos), m_container_type(container_type) {}

    // What this container spawns/holds
    ItemContainerBuilder& holds(EntityType item_type) {
        m_held_type = item_type;
        return *this;
    }

    // Indexer support (for cycling through variants like alcohols, fruits)
    ItemContainerBuilder& with_indexer(int size, int starting_value = 0) {
        m_indexer_size = size;
        m_indexer_start = starting_value;
        return *this;
    }

    // Generation limits (for items that shouldn't respawn infinitely)
    ItemContainerBuilder& max_generations(int count) {
        m_max_generations = count;
        return *this;
    }

    // Behavior when empty - acts like a table
    ItemContainerBuilder& table_when_empty() {
        m_table_when_empty = true;
        return *this;
    }

    // Item filters on the CanHoldItem component
    ItemContainerBuilder& with_filter(EntityFilter filter) {
        m_filter = std::move(filter);
        return *this;
    }

    // Finalize - applies all configuration
    void build() {
        furniture::make_itemcontainer(m_entity, {m_container_type}, m_pos,
                                      m_held_type);

        if (m_indexer_size > 0) {
            m_entity.addComponent<Indexer>(m_indexer_size)
                .set_value(m_indexer_start);
            m_entity.get<IsItemContainer>().set_uses_indexer(true);
        }

        if (m_max_generations >= 0) {
            m_entity.get<IsItemContainer>().set_max_generations(
                m_max_generations);
        }

        if (m_table_when_empty) {
            m_entity.get<IsItemContainer>().enable_table_when_enable();
        }

        if (m_filter) {
            m_entity.get<CanHoldItem>().set_filter(*m_filter);
        }
    }

   private:
    Entity& m_entity;
    vec2 m_pos;
    EntityType m_container_type;
    EntityType m_held_type = EntityType::Unknown;
    int m_indexer_size = 0;
    int m_indexer_start = 0;
    int m_max_generations = -1;
    bool m_table_when_empty = false;
    std::optional<EntityFilter> m_filter;
};
