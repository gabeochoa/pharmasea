
#pragma once

//
#include "base_component.h"

struct HasClientID : public BaseComponent {
    void update(int new_client_id) { client_id = new_client_id; }
    void update_ping(long long ping) { last_ping_to_server = ping; }

    [[nodiscard]] int id() const { return client_id; }
    [[nodiscard]] long long ping() const { return last_ping_to_server; }

   private:
    int client_id = -1;
    long long last_ping_to_server = -1;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.client_id,                 //
            self.last_ping_to_server        //
        );
    }
};
