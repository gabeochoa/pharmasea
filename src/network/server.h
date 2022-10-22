#pragma once

#include <cstring>

#include "../external_include.h"
//
#include "shared.h"

namespace network {
namespace server {

struct Internal : public BaseInternal {
    enum State {
        s_None,
        s_Init,
        s_Hosted,
        s_Error,
    } state;

    ENetAddress address;
    ENetEvent event;
    ENetHost* server = nullptr;
};

static bool ready(std::shared_ptr<Internal> internal) {
    return internal->state == Internal::State::s_Hosted;
}

static std::string status(std::shared_ptr<Internal> internal) {
    switch (internal->state) {
        case Internal::State::s_None:
            return "Not setup";
        case Internal::State::s_Init:
            return "Init but not hosting";
        case Internal::State::s_Hosted:
            return "Currently hosting";
        case Internal::State::s_Error:
            return "Error";
    };
    return "Invalid State";
}

static bool initialize(std::shared_ptr<Internal> internal) {
    int result = enet_initialize();
    if (result != 0) {
        std::cout << "failed to initialize network library" << std::endl;
        internal->state = Internal::State::s_Error;
        return false;
    }
    // Create the singleton
    internal->state = Internal::State::s_Init;
    return true;
}

static bool host(std::shared_ptr<Internal> internal) {
    internal->address.host =
        ENET_HOST_ANY; /* Bind the server to the default localhost.     */
    internal->address.port = DEFAULT_PORT; /* Bind the server to port 7777. */

    /* create a server */
    internal->server =
        enet_host_create(&internal->address, MAX_CLIENTS, 2, 0, 0);

    if (internal->server == nullptr) {
        printf(
            "An error occurred while trying to create an ENet server "
            "host.\n");
        internal->state = Internal::State::s_Error;
        return 1;
    }

    printf("Started a server...\n");
    internal->state = Internal::State::s_Hosted;
    return true;
}

static void poll(std::shared_ptr<Internal> internal, int time_ms) {
    /* Wait up to time milliseconds for an event. (WARNING: blocking) */
    while (enet_host_service(internal->server, &(internal->event), time_ms) >
           0) {
        switch (internal->event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                       internal->event.peer->address.host,
                       internal->event.peer->address.port);
                /* Store any relevant client information here. */
                internal->event.peer->data = (void*) "Client information";
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                // printf(
                // "A packet of length %lu containing %s was received "
                // "from %s "
                // "on channel %u.\n",
                // internal->event.packet->dataLength,
                // internal->event.packet->data,
                // (const char*) internal->event.peer->data,
                // internal->event.channelID);
                unsigned char* data = internal->event.packet->data;
                std::size_t length = internal->event.packet->dataLength;

                Buffer buffer;
                for (std::size_t i = 0; i < length; i++) {
                    buffer.push_back(data[i]);
                }
                ClientPacket packet;
                bitsery::quickDeserialization<InputAdapter>(
                    {buffer.begin(), length}, packet);
                process_packet(internal, packet);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(internal->event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n",
                       (const char*) internal->event.peer->data);
                /* Reset the peer's client information. */
                internal->event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("%s disconnected due to timeout.\n",
                       (const char*) internal->event.peer->data);
                /* Reset the peer's client information. */
                internal->event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

static void disconnect(std::shared_ptr<Internal> internal) {
    enet_host_destroy(internal->server);
    enet_deinitialize();
    internal->state = Internal::State::s_None;
}

}  // namespace server
}  // namespace network
