
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
        friend zpp::bits::access;
        constexpr static auto serialize(auto& archive, auto& self) {
            if (auto result = archive(  //
                    self.amount,        //
                    self.extra          //
                    );
                zpp::bits::failure(result)) {
                return result;
            }

            // animation stuff
            return archive(            //
                self.remainingTime,     //
                self.remainingTimeTotal //
            );
        }
    };

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

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        using archive_type = std::remove_cvref_t<decltype(archive)>;
        if (auto result = archive(                      //
                static_cast<BaseComponent&>(self),       //
                self.num_in_cart,                        //
                self.num_transactions,                   //
                self.transactions,                       //
                self.coins                               //
                );
            zpp::bits::failure(result)) {
            return result;
        }
        if constexpr (archive_type::kind() == zpp::bits::kind::in) {
            if (static_cast<int>(self.transactions.size()) > self.num_transactions) {
                return std::errc::message_size;
            }
        }
        return std::errc{};
    }
};
