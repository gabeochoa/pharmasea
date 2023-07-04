#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "base_component.h"

struct HasTimer : public BaseComponent {
    // TODO using this cause i dont want to make a new component for each
    // renderer type
    enum Renderer {
        Round,
    } type;

    virtual ~HasTimer() {}

    HasTimer() : type(Round), currentRoundTime(10.f), totalRoundTime(10.f) {}

    explicit HasTimer(Renderer _type, float t) {
        type = _type;
        totalRoundTime = t;
        currentRoundTime = totalRoundTime;
    }

    auto& pass_time(float dt) {
        if (currentRoundTime >= 0) currentRoundTime -= dt;
        return *this;
    }

    [[nodiscard]] float pct() const {
        return currentRoundTime / totalRoundTime;
    }

    // TODO make these private
    float currentRoundTime;
    float totalRoundTime;

    // TODO move into its own component
    enum WaitingReason {
        None,
        CustomersInStore,
        HoldingFurniture,
        WaitingReasonLast,
    } waiting_reason = None;

    std::bitset<WaitingReason::WaitingReasonLast> block_state_change_reasons;

    [[nodiscard]] std::string text_reason(WaitingReason wr) {
        switch (wr) {
            case CustomersInStore:
                return "Can't close until all customers leave";
            case HoldingFurniture:
                return "Can't start game until all players drop furniture";
            default:
            case WaitingReasonLast:
            case None:
                return "";
        }
    }

    [[nodiscard]] std::string text_reason(int i) {
        auto name = magic_enum::enum_value<WaitingReason>(i);
        return text_reason(name);
    }

    [[nodiscard]] bool read_reason(WaitingReason wr) const {
        return read_reason(magic_enum::enum_integer<WaitingReason>(wr));
    }

    [[nodiscard]] bool read_reason(int i) const {
        return block_state_change_reasons.test(i);
    }

    void write_reason(WaitingReason wr, bool value) {
        auto i = magic_enum::enum_integer<WaitingReason>(wr);
        block_state_change_reasons[i] = value;
    }

    void on_complete() {
        auto _reset_timer = [&]() { currentRoundTime = totalRoundTime; };

        switch (GameState::get().read()) {
            case game::State::InRoundClosing: {
                // For this one, we need to wait until everyone is done leaving
                if (read_reason(WaitingReason::CustomersInStore)) return;
                GameState::get().set(game::State::Planning);
            } break;
            case game::State::Planning: {
                // For this one, we need to wait until everyone drops the things
                if (read_reason(WaitingReason::HoldingFurniture)) return;
                GameState::get().set(game::State::InRound);
            } break;
            case game::State::InRound: {
                GameState::get().set(game::State::InRoundClosing);
            } break;

            default:
                log_warn("Completed timer but no state handler {}",
                         GameState::get().read());
                return;
        }
        _reset_timer();
    }
    void reset_if_complete() {
        if (currentRoundTime <= 0.f) on_complete();
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        //
        s.ext(block_state_change_reasons, bitsery::ext::StdBitset{});

        s.value4b(type);
        s.value4b(currentRoundTime);
        s.value4b(totalRoundTime);
    }
};
