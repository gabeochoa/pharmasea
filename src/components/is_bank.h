
#pragma once

#include "base_component.h"

struct IsBank : public BaseComponent {
    struct Transaction {
        Transaction(int amt, int xtra)
            : amount(amt), extra(xtra), remainingTimeTotal(0.75f) {
            remainingTime = remainingTimeTotal;
        }
        explicit Transaction(int amt) : Transaction(amt, 0) {}

        int amount;
        int extra;

        // TODO is there a way for the render to use the pointer as a unique
        // key? then we can do this without storing all this stuff on us?

        // animation stuff
        float remainingTime;
        float remainingTimeTotal;

        [[nodiscard]] float pct() const {
            return remainingTime / remainingTimeTotal;
        }

        // This exists for serialization
        Transaction()
            : amount(0),
              extra(0),
              remainingTime(0.f),
              remainingTimeTotal(0.f) {}
        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value4b(amount);
            s.value4b(extra);

            // animation stuff
            s.value4b(remainingTime);
            s.value4b(remainingTimeTotal);
        }
    };

    virtual ~IsBank() {}
    IsBank() {}

    [[nodiscard]] int balance() const { return coins; }
    [[nodiscard]] int cart() const { return num_in_cart; }
    [[nodiscard]] std::vector<Transaction>& get_transactions() {
        num_transactions = (int) transactions.size();
        return transactions;
    }

    [[nodiscard]] const std::vector<Transaction>& get_transactions() const {
        return transactions;
    }

    [[nodiscard]] Transaction& get_next_transaction() {
        return transactions.front();
    }

    void deposit_with_tip(int amount, int tip) {
        coins += amount;
        coins += tip;
        transactions.push_back(Transaction(amount, tip));
        num_transactions++;
    }

    void deposit(int amt) {
        coins += amt;
        transactions.push_back(Transaction(amt));
        num_transactions++;
    }

    void withdraw(int amt) {
        coins -= amt;
        transactions.push_back(Transaction(-amt));
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
    std::vector<Transaction> transactions;
    int coins = 0;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(num_in_cart);

        s.value4b(num_transactions);
        s.container(transactions, num_transactions,
                    [](S& s2, Transaction& a) { s2.object(a); });

        s.value4b(coins);
    }
};
