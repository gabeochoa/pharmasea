
#pragma once

#include "external_include.h"
#include <cstring>

namespace network {

const int DEFAULT_PORT = 7777;
const int MAX_CLIENTS = 32;
namespace server {

struct Internal {
    enum State {
        s_None,
        s_Init,
        s_Hosted,
        s_Error,
    } state;

    ENetAddress address;
    ENetEvent event;
    ENetHost* server = nullptr;

    struct Client {};
    std::vector<Client*> clients;
};

static bool ready(std::shared_ptr<Internal> internal) {
    return internal->state == Internal::State::s_Hosted;
}

static bool has_clients(std::shared_ptr<Internal> internal) {
    return internal->clients.size();
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

            case ENET_EVENT_TYPE_RECEIVE:
                printf(
                    "A packet of length %lu containing %s was received "
                    "from %s "
                    "on channel %u.\n",
                    internal->event.packet->dataLength,
                    internal->event.packet->data,
                    (const char*) internal->event.peer->data,
                    internal->event.channelID);
                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(internal->event.packet);
                break;

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

/*
static bool server_all() {
    network::server::std::shared_ptr<Internal> internal = new Internal();
    bool init = network::server::initialize(internal);
    if (!init) {
        return 0;
    }
    bool connect = network::server::host(internal);
    if (!connect) {
        return 0;
    }

    network::server::poll(internal, 10);
    network::server::disconnect(internal);

    delete internal;
    return 0;
}
*/

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

static void send(std::shared_ptr<Internal> internal) {
    /* Create a reliable packet of size 7 containing "packet\0" */
    ENetPacket* packet = enet_packet_create("packet", strlen("packet") + 1,
                                            ENET_PACKET_FLAG_RELIABLE);
    /* Send the packet to the peer over channel id 0. */
    /* One could also broadcast the packet by         */
    /* enet_host_broadcast (host, 0, packet);         */
    enet_peer_send(internal->peer, 0, packet);
}

}  // namespace client

/*
static int client_all() {
    network::server::std::shared_ptr<Internal> internal = new Internal();
    bool init = network::client::initialize(internal);
    if (!init) {
        return 0;
    }
    bool connect = network::client::connect(internal);
    if (!connect) {
        return 0;
    }

    network::client::recieve_event(internal, 5000);
    network::client::disconnect(internal);

    delete internal;
    return 0;
}
*/

struct Info {
    enum State { s_None, s_Host, s_Client } desired_role = s_None;
    std::shared_ptr<client::Internal> client_info;
    std::shared_ptr<server::Internal> server_info;

    Info() { reset(); }
    ~Info() {}

    void reset() {
        client_info.reset(new client::Internal());
        server_info.reset(new server::Internal());
    }
    void set_role_to_client() {
        if (!client::initialize(this->client_info)) {
            return;
        }
        if (!client::connect(this->client_info)) {
            return;
        }
        this->desired_role = Info::State::s_Client;
    }

    void set_role_to_none() {
        this->desired_role = Info::State::s_None;

        if (this->client_info->state != client::Internal::State::s_None) {
            network::client::disconnect(this->client_info);
        }

        if (this->server_info->state != server::Internal::State::s_None) {
            network::server::disconnect(this->server_info);
        }
    }

    void set_role_to_host() {
        if (!server::initialize(this->server_info)) {
            return;
        }
        if (!server::host(this->server_info)) {
            return;
        }
        this->desired_role = Info::State::s_Host;
    }

    bool is_host() { return this->desired_role == Info::State::s_Host; }

    bool is_client() { return this->desired_role == Info::State::s_Client; }

    std::string status() {
        if (is_host()) {
            return server::status(this->server_info);
        }
        if (is_client()) {
            return client::status(this->client_info);
        }
        return "";
    }

    void disconnect() {
        if (is_host()) {
            server::disconnect(this->server_info);
        }
        if (is_client()) {
            client::disconnect(this->client_info);
        }
    }

    void server_tick(float, int time_ms) {
        if (this->server_info->state != server::Internal::State::s_Hosted)
            return;
        server::poll(this->server_info, time_ms);
    }

    void client_tick(float, int time_ms) {
        if (this->client_info->state != client::Internal::State::s_Connected)
            return;
        client::recieve_event(this->client_info, time_ms);
    }
};

}  // namespace network
