
#pragma once

#include "base_component.h"

struct IsBank : public BaseComponent {
    virtual ~IsBank() {}
    IsBank() {}

    [[nodiscard]] int balance() const { return coins; }
    [[nodiscard]] int cart() const { return num_in_cart; }

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

    void update_cart(int c) { num_in_cart = c; }

   private:
    int num_in_cart = 0;

    int num_transactions = 0;
    std::vector<int> transactions;
    int coins = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(num_in_cart);

        s.value4b(num_transactions);
        s.container(transactions, num_transactions,
                    [](S& s2, int a) { s2.value4b(a); });

        s.value4b(coins);
    }
};
