#pragma once

#include "../engine/log.h"
#include "../entity_query.h"
#include "ai_component.h"
#include "base_component.h"
#include "has_waiting_queue.h"

struct AIPlayJukebox : public AIComponent {
    struct AIPlayJukeboxTarget : AITarget {
        explicit AIPlayJukeboxTarget(const ResetFn& resetFn)
            : AITarget(resetFn) {}

        virtual OptEntity find_target(const Entity&) override {
            return EntityQuery()
                .whereType(EntityType::Jukebox)
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full jukeboxs
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                // Find the jukebox with the least people on it
                .orderByLambda([](const Entity& r1, const Entity& r2) {
                    const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
                    int rpos1 = hwq1.get_next_pos();
                    const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
                    int rpos2 = hwq2.get_next_pos();
                    return rpos1 < rpos2;
                })
                .gen_first();
        }
    } target;

    AILineWait line_wait;

    AIPlayJukebox()
        : target(std::bind(&AIComponent::reset, this)),
          line_wait(std::bind(&AIComponent::reset, this)) {}
    virtual ~AIPlayJukebox() {}

    float findSongTime = -1;
    void set_findSong_time(float pt) { findSongTime = pt; }
    [[nodiscard]] bool findSong(float dt) {
        findSongTime -= dt;
        return findSongTime <= 0.f;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<AIComponent>{});
    }
};