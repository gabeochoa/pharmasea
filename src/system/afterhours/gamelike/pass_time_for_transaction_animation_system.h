#pragma once

#include "../../../ah.h"
#include "../../../components/is_bank.h"
#include "../../../engine/statemanager.h"

namespace system_manager {

struct PassTimeForTransactionAnimationSystem
    : public afterhours::System<IsBank> {
    virtual bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    virtual void for_each_with(Entity&, IsBank& bank, float dt) override {
        std::vector<IsBank::Transaction>& transactions =
            bank.get_transactions();

        // Remove any old ones
        remove_all_matching<IsBank::Transaction>(
            transactions, [](const IsBank::Transaction& transaction) {
                return transaction.remainingTime <= 0.f ||
                       transaction.amount == 0;
            });

        if (transactions.empty()) return;

        IsBank::Transaction& transaction = bank.get_next_transaction();
        transaction.remainingTime -= dt;
    }
};

}  // namespace system_manager