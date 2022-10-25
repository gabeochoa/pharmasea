

#pragma once

#include <yojimbo/yojimbo.h>

#include <iostream>

#include "shared.h"

namespace network {

struct PharmaSeaClient {
    yojimbo::Adapter adapter;
    yojimbo::Client client;
    yojimbo::ClientServerConfig connectionConfig;

    PharmaSeaClient(const yojimbo::Address& serverAddress)
        : client(yojimbo::GetDefaultAllocator(), serverAddress,
                 connectionConfig, adapter, 0.0) {
        uint64_t clientId;
        yojimbo::random_bytes((uint8_t*) &clientId, 8);
        client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, serverAddress);
    }

    void update(float dt) {
        client.AdvanceTime(client.GetTime() + dt);
        client.ReceivePackets();

        if (client.IsConnected()) {
            process_messages();
        }

        client.SendPackets();
    }

    void process_message(yojimbo::Message* message) {
        auto process_message_test = [](TestMessage* message) {
            std::cout << "got test message from " << message->data << std::endl;
        };

        switch (message->GetType()) {
            case (int) PharmaSeaMessageType::TEST:
                process_message_test((TestMessage*) message);
                break;
            default:
                break;
        }
    }

    void process_messages() {
        for (int i = 0; i < connectionConfig.numChannels; i++) {
            yojimbo::Message* message = client.ReceiveMessage(i);
            while (message != NULL) {
                process_message(message);
                client.ReleaseMessage(message);
                message = client.ReceiveMessage(i);
            }
        }
    }
};

}  // namespace network
