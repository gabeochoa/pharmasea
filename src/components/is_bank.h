
#pragma once

#include "base_component.h"

struct IsBank : public BaseComponent {
    virtual ~IsBank() {}
    IsBank() {}

    [[nodiscard]] int balance() const { return coins; }
    void deposit(int amt) {
        coins += amt;
        transactions.push_back(amt);
        num_transactions++;
    }
    void withdraw(int amt) {
        coins -= amt;
        transactions.push_back(-amt);
        num_transactions++;
    }

    void clear_transactions() {
        transactions.clear();
        num_transactions = 0;
    }

   private:
    int num_transactions = 0;
    std::vector<int> transactions;
    int coins = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(num_transactions);
        s.container(transactions, num_transactions,
                    [](S& s2, int a) { s2.value4b(a); });
        s.value4b(coins);
    }
};
