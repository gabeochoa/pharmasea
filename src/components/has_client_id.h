
#pragma once

//
#include "base_component.h"

struct HasClientID : public BaseComponent {
    int client_id = -1;

    virtual ~HasClientID() {}

    void update(int new_client_id) { client_id = new_client_id; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        s.value4b(client_id);
    }
};
