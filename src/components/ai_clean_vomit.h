#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "ai_component.h"
#include "base_component.h"

struct AICleanVomit : public AIComponent {
    struct AICleanVomitTarget : AITarget {
        explicit AICleanVomitTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity& entity) override {
            // issues with this one:
            // TODO often the first person to see it grabs it even if they arent
            // the closest
            // TODO once  num vomit < num mops they just stop moving, they
            // should probably continue

            auto other_ais = EntityQuery()
                                 .whereNotID(entity.id)
                                 .whereHasComponent<AICleanVomit>()
                                 .gen();
            std::set<int> existing_targets;
            for (const Entity& mop : other_ais) {
                auto id = mop.get<AICleanVomit>().target.target_id;
                if (!id.has_value()) continue;
                existing_targets.insert(id.value());
            }

            bool more_boys_than_vomit =
                existing_targets.size() < other_ais.size();

            return EntityQuery()
                .whereType(EntityType::Vomit)
                .whereLambda([&](const Entity& vomit) {
                    // if theres almost no vomit left, then just go try to clean
                    // it
                    if (more_boys_than_vomit) return true;
                    // Skip the ones other people are targetting
                    return !existing_targets.contains(vomit.id);
                })
                .orderByDist(entity.get<Transform>().as2())
                .gen_first();
        }
    } target;

    AICleanVomit() : target(std::bind(&AIComponent::reset, this)) {}

    virtual ~AICleanVomit() {}

   public:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<AIComponent>(this), target);
    }
};

CEREAL_REGISTER_TYPE(AICleanVomit);
