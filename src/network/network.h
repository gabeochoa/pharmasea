
#pragma once

#include <cstring>

#include "../../vendor/bitsery/serializer.h"
#include "../../vendor/enetpp.h"
//
#include "../external_include.h"
// //
#include "../player.h"
#include "../remote_player.h"
#include "../random.h"

namespace network {

struct ThinClient {
    int _uid;
    int get_id() const { return _uid; }
};

struct Info {
    enum State {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    enetpp::server<ThinClient> server;
    enetpp::client client;

    Info() { enetpp::global_state::get().initialize(); }

    ~Info() {
        close_active_roles();
        enetpp::global_state::get().deinitialize();
    }

    void close_active_roles() {
        if (desired_role & s_Client) client.disconnect();
        if (desired_role & s_Host) server.stop_listening();
    }

    void set_role_to_client() {
        close_active_roles();
        this->desired_role = Info::State::s_Client;
    }

    void set_role_to_none() {
        close_active_roles();
        this->desired_role = Info::State::s_None;
    }

    void set_role_to_host() {
        close_active_roles();
        this->desired_role = Info::State::s_Host & Info::State::s_Client;
    }

    bool is_host() { return this->desired_role & Info::State::s_Host; }

    bool is_client() { return this->desired_role & Info::State::s_Client; }
    bool has_role() { return is_host() || is_client(); }

    void start_server() {
        auto init_client_func = [](ThinClient& thin_client, const char* ip) {
            auto h1 =
                std::hash<std::string>{}(std::string(ip)) + randIn(0, 1000);
            thin_client._uid = h1;
            std::cout << "last client id was: " << h1 << std::endl;
        };

        server.set_trace_handler([](const std::string& line) {
            std::cout << "server: " << line << std::endl;
        });

        server.start_listening(
            enetpp::server_listen_params<ThinClient>()
                .set_max_client_count(3)
                .set_channel_count(1)
                .set_listen_port(770)
                .set_initialize_client_function(init_client_func));

        // consume events raised by worker thread
        auto on_client_connected = [&](ThinClient&) {
            std::cout << "server::connected" << std::endl;
        };
        auto on_client_disconnected = [&](unsigned int) {
            std::cout << "server::disconnected" << std::endl;
        };
        auto on_client_data_received = [&](ThinClient& packet_client,
                                           const enet_uint8* data,
                                           size_t data_size) {
            std::cout << "server::data '" +
                             std::string(reinterpret_cast<const char*>(data),
                                         data_size) +
                             "'"
                      << std::endl;
            std::cout << "forwarding packet to all other clients..."
                      << std::endl;
            server.send_packet_to_all_if(
                0, data, data_size, ENET_PACKET_FLAG_RELIABLE,
                [&](const ThinClient& destination) {
                    return destination.get_id() != packet_client.get_id();
                });
        };

        server.register_callbacks("server", on_client_connected,
                                  on_client_disconnected,
                                  on_client_data_received);
    }

    void start_client() {
        client.set_trace_handler([](const std::string& line) {
            std::cout << "client: " << line << std::endl;
        });

        client.connect(enetpp::client_connect_params()
                           .set_channel_count(1)
                           .set_server_host_name_and_port("localhost", 770));

        // consume events raised by worker thread
        auto on_connected = [&]() {
            std::cout << "client::connected" << std::endl;
        };
        auto on_disconnected = [&]() {
            std::cout << "client::disconnected" << std::endl;
        };
        auto on_data_received = [&](const enet_uint8* data, size_t data_size) {
            std::cout << "client::data" << std::endl;
            std::cout << ("received packet from server : '" +
                          std::string(reinterpret_cast<const char*>(data),
                                      data_size) +
                          "'")
                      << std::endl;
        };

        client.register_callbacks("client", on_connected, on_disconnected,
                                  on_data_received);
    }

    void network_tick(float dt) {
        auto network_client = [&](float) {
            if (!client.is_connecting_or_connected()) {
                return;
            }

            std::string packet = "hello from client";
            assert(sizeof(char) == sizeof(enet_uint8));
            client.send_packet(
                0, reinterpret_cast<const enet_uint8*>(packet.data()),
                packet.length(), ENET_PACKET_FLAG_RELIABLE);

            client.consume_events();
        };
        if (desired_role & s_Client) network_client(dt);

        auto network_host = [&](float) {
            if (!server.is_listening()) return;
            // send stuff to specific client where uid=123
            std::string packet = "data_to_send";
            assert(sizeof(char) == sizeof(enet_uint8));
            // server.send_packet_to(123, 0, &data_to_send, 1,
            // ENET_PACKET_FLAG_RELIABLE);
            //
            // send stuff to all clients (with optional predicate filter)
            server.send_packet_to_all_if(
                0, reinterpret_cast<const enet_uint8*>(packet.data()),
                packet.length(), ENET_PACKET_FLAG_RELIABLE,
                [](const ThinClient&) { return true; });

            server.consume_events();
            // get access to all connected clients
            // for (auto c : server.get_connected_clients()) { }
        };
        if (desired_role & s_Host) network_host(dt);
    }
};

}  // namespace network
