
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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                client_id, last_ping_to_server);
    }
};

CEREAL_REGISTER_TYPE(HasClientID);
