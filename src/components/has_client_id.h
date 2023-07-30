
#pragma once

//
#include "base_component.h"

struct HasClientID : public BaseComponent {
    virtual ~HasClientID() {}

    void update(int new_client_id) { client_id = new_client_id; }
    void update_ping(long long ping) { last_ping_to_server = ping; }

    [[nodiscard]] int id() const { return client_id; }
    [[nodiscard]] long long ping() const { return last_ping_to_server; }

   private:
    int client_id = -1;
    long long last_ping_to_server = -1;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(client_id);
        s.value8b(last_ping_to_server);
    }
};
