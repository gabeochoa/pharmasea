
#pragma once

#include <fmt/format.h>
#include <yojimbo/yojimbo.h>

#include <iostream>
#include <string>

#include "shared.h"

namespace network {

// the client and server config
struct PharmaSeaConnectionConfig : yojimbo::ClientServerConfig {
    PharmaSeaConnectionConfig() {
        numChannels = 2;
        channel[(int) PharmaSeaChannel::RELIABLE].type =
            yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
        channel[(int) PharmaSeaChannel::UNRELIABLE].type =
            yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
    }
};

struct PharmaSeaServer;

struct PharmaSeaAdapter : public yojimbo::Adapter {
    PharmaSeaServer* ps_server;

    explicit PharmaSeaAdapter(PharmaSeaServer* server = NULL)
        : ps_server(server) {}

    yojimbo::MessageFactory* CreateMessageFactory(
        yojimbo::Allocator& allocator) override {
        return YOJIMBO_NEW(allocator, PharmaSeaMessageFactory, allocator);
    }

    virtual void OnServerClientConnected(int client_id) override;
    virtual void OnServerClientDisconnected(int client_id) override;
};

struct PharmaSeaServer {
    // NOTE: config constructor has to run first, otherwise serverGlobalMemory
    //       is 0 and the allocator will assert on startup.
    yojimbo::ClientServerConfig connectionConfig;
    PharmaSeaAdapter adapter;
    yojimbo::Server server;
    float time = 0.f;
    float total_time = 0.f;

    PharmaSeaServer(const yojimbo::Address& address)
        : adapter(this),
          server(yojimbo::GetDefaultAllocator(), DEFAULT_PRIVATE_KEY, address,
                 connectionConfig, adapter, 0.0) {
        server.Start(MAX_PLAYERS);
        if (!server.IsRunning()) {
            throw std::runtime_error(fmt::format(
                "Could not start server at port {}", address.GetPort()));
        }

        char buffer[256];
        server.GetAddress().ToString(buffer, sizeof(buffer));
        std::cout << "Server address is " << buffer << std::endl;
    }

    void client_connected(int client_id) {
        std::cout << "client " << client_id << " connected" << std::endl;
    }

    void client_disconnected(int client_id) {
        std::cout << "client " << client_id << " disconnected" << std::endl;
    }

    void update(float dt) {
        auto tick = [&]() {
            if (!server.IsRunning()) {
                return false;
            }
            server.AdvanceTime(total_time);
            server.ReceivePackets();
            process_messages();
            return true;
        };

        float fixed_dt = 1.f / 60.f;
        time -= dt;
        if (time <= 0) {
            tick();
            time += fixed_dt;
            total_time += fixed_dt;
        }
    }

    void process_message(int client_index, yojimbo::Message* message) {
        auto process_message_test = [&](int client_index,
                                        TestMessage* message) {
            std::cout << "got test message from " << client_index << std::endl;
            TestMessage* testMessage = (TestMessage*) server.CreateMessage(
                client_index, (int) PharmaSeaMessageType::TEST);

            testMessage->data = message->data;
            server.SendMessage(client_index, (int) PharmaSeaChannel::RELIABLE,
                               testMessage);
        };

        switch (message->GetType()) {
            case (int) PharmaSeaMessageType::TEST:
                process_message_test(client_index, (TestMessage*) message);
                break;
            default:
                break;
        }
    }

    void process_messages() {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (server.IsClientConnected(i)) {
                for (int j = 0; j < connectionConfig.numChannels; j++) {
                    yojimbo::Message* message = server.ReceiveMessage(i, j);
                    while (message != NULL) {
                        process_message(i, message);
                        server.ReleaseMessage(i, message);
                        message = server.ReceiveMessage(i, j);
                    }
                }
            }
        }
    }
};

}  // namespace network
