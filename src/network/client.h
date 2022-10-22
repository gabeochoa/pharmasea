#pragma once

#include <cstring>

#include "../external_include.h"
//
#include "shared.h"

namespace network {
namespace client {

struct Internal {
    ENetHost* client = nullptr;
    ENetPeer* peer = nullptr;

    ENetAddress address;
    ENetEvent event;

    enum State {
        s_None,
        s_Init,
        s_Connected,
        s_Error,
    } state;
};

static std::string status(std::shared_ptr<Internal> internal) {
    switch (internal->state) {
        case Internal::State::s_None:
            return "Not setup";
        case Internal::State::s_Init:
            return "Init but not hosting";
        case Internal::State::s_Connected:
            return "Currently connected";
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

    internal->client =
        enet_host_create(nullptr /* create a client host */,
                         1 /* only allow 1 outgoing connection */,
                         2 /* allow up 2 channels to be used, 0 and 1 */,
                         0 /* assume any amount of incoming bandwidth */,
                         0 /* assume any amount of outgoing bandwidth */);

    if (internal->client == nullptr) {
        std::cout << "failed to create client socket" << std::endl;
        internal->state = Internal::State::s_Error;
        return false;
    }

    internal->state = Internal::State::s_Init;
    return true;
}

static bool connect(std::shared_ptr<Internal> internal) {
    /* Connect to some.server.net:1234. */
    enet_address_set_host(&internal->address, "127.0.0.1");
    internal->address.port = DEFAULT_PORT;
    /* Initiate the connection, allocating the two channels 0 and 1. */
    internal->peer =
        enet_host_connect(internal->client, &internal->address, 2, 0);
    if (internal->peer == nullptr) {
        std::cout << "No available peers for initiating an ENet connection.\n"
                  << std::endl;
        internal->state = Internal::State::s_Error;
        return false;
    }

    int result = enet_host_service(internal->client, &internal->event, 5000);
    if (result > 0 && internal->event.type == ENET_EVENT_TYPE_CONNECT) {
        /* Wait up to 5 seconds for the connection attempt to succeed. */
        internal->state = Internal::State::s_Connected;
        return true;
    }
    /* Either the 5 seconds are up or a disconnect event was */
    /* received. Reset the peer in the event the 5 seconds   */
    /* had run out without any significant event.            */
    enet_peer_reset(internal->peer);
    internal->state = Internal::State::s_Error;
    std::cout << "Didnt find server in time" << std::endl;
    return false;
}

static bool recieve_event(std::shared_ptr<Internal> internal, int time_ms) {
    // Receive some events
    if (enet_host_service(internal->client, &internal->event, time_ms) == 0) {
        return false;
    }

    std::cout << "recieved event" << std::endl;

    switch (internal->event.type) {
        case ENET_EVENT_TYPE_RECEIVE:
            std::cout << fmt::format(
                             "A packet of length %lu containing %s was "
                             "received from %s on "
                             "channel %u.\n",
                             internal->event.packet->dataLength,
                             internal->event.packet->data,
                             (char*) internal->event.peer->data,
                             internal->event.channelID)
                      << std::endl;
            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(internal->event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            std::cout << fmt::format("%s disconnected.\n",
                                     (const char*) internal->event.peer->data)
                      << std::endl;
            /* Reset the peer's client information. */
            internal->event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            std::cout << fmt::format("%s disconnected due to timeout.\n",
                                     (const char*) internal->event.peer->data)
                      << std::endl;
            /* Reset the peer's client information. */
            internal->event.peer->data = NULL;
            break;

        case ENET_EVENT_TYPE_NONE:
            break;
        default:
            std::cout << "got other event" << std::endl;
            break;
    }
    return true;
}

static bool disconnect(std::shared_ptr<Internal> internal) {
    // Disconnect
    enet_peer_disconnect(internal->peer, 0);

    uint8_t disconnected = false;
    /* Allow up to 3 seconds for the disconnect to succeed
     * and drop any packets received packets.
     */
    while (enet_host_service(internal->client, &internal->event, 3000) > 0) {
        switch (internal->event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(internal->event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                puts("Disconnection succeeded.");
                disconnected = true;
                break;
            default:
                break;
        }
    }

    // Drop connection, since disconnection didn't successed
    if (!disconnected) {
        enet_peer_reset(internal->peer);
    }

    enet_host_destroy(internal->client);
    enet_deinitialize();
    internal->state = Internal::State::s_None;
    return true;
}

static void send(std::shared_ptr<Internal> internal,
                 const unsigned char* buffer, std::size_t buffer_size) {
    ENetPacket* packet =
        enet_packet_create(buffer, buffer_size, ENET_PACKET_FLAG_RELIABLE);
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send(internal->peer, 0, packet);
}

}  // namespace client
}  // namespace network
