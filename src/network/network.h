
#pragma once

#include <cstring>

#include "../../vendor/bitsery/serializer.h"
#include "../../vendor/enetpp.h"
//
#include "../external_include.h"
// //
#include "../player.h"
#include "../random.h"
#include "../remote_player.h"
#include "enet.h"
#include "shared.h"

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
    int my_client_id = -1;
    // std::wstring username = L"葛城 ミサト";
    std::wstring username = L"";
    bool username_set = false;

    std::map<int, std::shared_ptr<RemotePlayer>> remote_players;

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
        this->desired_role = Info::State::s_Host;
    }

    bool is_host() { return this->desired_role & Info::State::s_Host; }

    bool is_client() { return this->desired_role & Info::State::s_Client; }
    bool has_role() { return is_host() || is_client(); }

    void fwd_packet_to_other_clients(int source_id, ClientPacket packet) {
        Buffer buffer;
        bitsery::quickSerialization(OutputAdapter{buffer}, packet);
        server.send_packet_to_all_if(
            0, buffer.data(), buffer.size(), ENET_PACKET_FLAG_RELIABLE,
            [&](const ThinClient& destination) {
                return destination.get_id() != source_id;
            });
    }

    void fwd_packet_to_source(int source_id, ClientPacket packet) {
        Buffer buffer;
        bitsery::quickSerialization(OutputAdapter{buffer}, packet);
        server.send_packet_to_all_if(
            0, buffer.data(), buffer.size(), ENET_PACKET_FLAG_RELIABLE,
            [&](const ThinClient& destination) {
                return destination.get_id() == source_id;
            });
    }

    void process_client_packet_msg(int client_id, ClientPacket packet) {
        auto add_remote_player = [&](int id) {
            if (is_host()) id = client_id;

            remote_players[id] = std::make_shared<RemotePlayer>();
            auto rp = remote_players[id];
            rp->client_id = id;
            EntityHelper::addEntity(remote_players[id]);
            std::cout << fmt::format(" Adding a player {}, {}", client_id, id)
                      << std::endl;
        };

        auto player_join = [&](ClientPacket::PlayerJoinInfo& player_join_info) {
            // NOTE: for this message type, the packet.client_id,
            // isnt yet filled in correctly and is likely -1
            // so we use the client_id in the message instead
            if (player_join_info.is_you) {
                // im the player
                my_client_id = player_join_info.client_id;
                return;
            }

            // Check to see if we already have this player?
            if (!remote_players.contains(packet.client_id)) {
                add_remote_player(packet.client_id);
            }
            return;
        };

        auto player_location = [&]() {
            ClientPacket::PlayerInfo player_info =
                std::get<ClientPacket::PlayerInfo>(packet.msg);
            int id = packet.client_id;
            if (is_host()) id = client_id;

            // Check to see if we already have this player?
            if (!remote_players.contains(id)) {
                add_remote_player(id);
            }

            auto rp = remote_players[id];
            if (!rp)
                std::cout << fmt::format("doesnt exist but should {}", id)
                          << std::endl;
            rp->update_remotely(player_info.name, player_info.location,
                                player_info.facing_direction);
        };

        auto game_state = [&]() {
            ClientPacket::GameStateInfo game_state =
                std::get<ClientPacket::GameStateInfo>(packet.msg);
            Menu::get().state = game_state.host_menu_state;
        };

        if (is_client()) {
            switch (packet.msg_type) {
                case ClientPacket::MsgType::PlayerJoin: {
                    ClientPacket::PlayerJoinInfo player_join_info =
                        std::get<ClientPacket::PlayerJoinInfo>(packet.msg);
                    player_join(player_join_info);
                } break;
                case ClientPacket::MsgType::PlayerLocation:
                    player_location();
                    break;
                case ClientPacket::MsgType::GameState:
                    game_state();
                    break;
                default:
                    break;
            }
            return;
        } else if (is_host()) {
            switch (packet.msg_type) {
                case ClientPacket::MsgType::PlayerJoin: {
                    ClientPacket::PlayerJoinInfo join_info =
                        std::get<ClientPacket::PlayerJoinInfo>(packet.msg);
                    // since we are the host we always get the player join from
                    // the source. So we can edit the original packet to inject
                    // the client's id
                    packet.client_id = client_id;
                    join_info.client_id = client_id;
                    packet.msg = join_info;

                    // we process it now
                    player_join(join_info);

                    // Send the source player information about their client id
                    ClientPacket::PlayerJoinInfo for_source(join_info);
                    for_source.is_you = true;
                    fwd_packet_to_source(
                        client_id,
                        ClientPacket(
                            {.client_id = my_client_id,
                             .msg_type = ClientPacket::MsgType::PlayerJoin,
                             .msg = for_source}));
                } break;
                case ClientPacket::MsgType::PlayerLocation:
                    player_location();
                    break;
                case ClientPacket::MsgType::GameState:
                // We do nothing for gamestate, since we are the source of
                // truth
                default:
                    break;
            }

            // default forwarding;
            fwd_packet_to_other_clients(client_id, packet);
            return;
        }
    }

    void start_server() {
        auto init_client_func = [](ThinClient& thin_client, const char* ip) {
            auto h1 =
                std::hash<std::string>{}(std::string(ip)) + randIn(0, 1000);
            thin_client._uid = (int) h1;
            std::cout << "last client id was: " << h1 << std::endl;
        };
        my_client_id = -2;

        server.set_trace_handler([](const std::string& line) {
            std::cout << "server: " << line << std::endl;
        });

        server.start_listening(
            enetpp::server_listen_params<ThinClient>()
                .set_max_client_count(MAX_CLIENTS)
                .set_channel_count(1)
                .set_listen_port(DEFAULT_PORT)
                .set_initialize_client_function(init_client_func));

        // consume events raised by worker thread
        auto on_client_connected = [&](ThinClient&) {
            std::cout << "server::connected" << std::endl;
        };
        auto on_client_disconnected = [&](unsigned int client_id) {
            std::cout << "server::disconnected" << std::endl;
            if (!remote_players.contains(client_id)) {
                return;
            }

            std::shared_ptr<RemotePlayer> r = remote_players[client_id];
            r->cleanup = true;
            remote_players.erase(client_id);
        };
        auto on_client_data_received = [&](ThinClient& source_client,
                                           const enet_uint8* data,
                                           size_t data_size) {
            // local processing
            Buffer buffer;
            for (std::size_t i = 0; i < data_size; i++)
                buffer.push_back(data[i]);
            ClientPacket packet;
            bitsery::quickDeserialization<InputAdapter>(
                {buffer.begin(), data_size}, packet);
            process_client_packet_msg(source_client.get_id(), packet);
        };

        server.register_callbacks("server", on_client_connected,
                                  on_client_disconnected,
                                  on_client_data_received);
    }

    void start_client() {
        client.set_trace_handler([](const std::string& line) {
            std::cout << "client: " << line << std::endl;
        });

        client.connect(
            enetpp::client_connect_params()
                .set_channel_count(1)
                .set_server_host_name_and_port("localhost", DEFAULT_PORT));

        // consume events raised by worker thread
        auto on_connected = [&]() {
            std::cout << "client::connected" << std::endl;
            ClientPacket connected_packet({
                // We dont yet know what it is but will soon
                .client_id = -1,
                .msg_type = ClientPacket::MsgType::PlayerJoin,
                .msg = ClientPacket::PlayerJoinInfo({
                    .is_you = false,
                    .client_id = -1,
                }),
            });
            Buffer buffer;
            bitsery::quickSerialization(OutputAdapter{buffer},
                                        connected_packet);
            client.send_packet(0, buffer.data(), buffer.size(),
                               ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
        };

        auto on_disconnected = [&]() {
            std::cout << "client::disconnected" << std::endl;
        };
        auto on_data_received = [&](const enet_uint8* data, size_t data_size) {
            // std::cout << "client::data" << std::endl;
            // std::cout << ("received packet from server : '" +
            // std::string(reinterpret_cast<const char*>(data),
            // data_size) +
            // "'")
            // << std::endl;
            Buffer buffer;
            for (std::size_t i = 0; i < data_size; i++)
                buffer.push_back(data[i]);
            ClientPacket packet;
            bitsery::quickDeserialization<InputAdapter>(
                {buffer.begin(), data_size}, packet);
            process_client_packet_msg(0, packet);
        };

        client.register_callbacks("client", on_connected, on_disconnected,
                                  on_data_received);
    }

    Buffer get_player_packet() {
        Player me = GLOBALS.get<Player>("player");
        ClientPacket player({
            .client_id = my_client_id,
            .msg_type = ClientPacket::MsgType::PlayerLocation,
            .msg = ClientPacket::PlayerInfo({
                .name = username,
                .location =
                    {
                        me.position.x,
                        me.position.y,
                        me.position.z,
                    },
                .facing_direction = static_cast<int>(me.face_direction),
            }),
        });

        Buffer buffer;
        bitsery::quickSerialization(OutputAdapter{buffer}, player);
        return buffer;
    }

    void client_send_player() {
        Buffer buffer = get_player_packet();
        client.send_packet(0, buffer.data(), buffer.size(),
                           ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
    }

    void host_send_player() {
        Buffer buffer = get_player_packet();
        server.send_packet_to_all_if(0, buffer.data(), buffer.size(),
                                     ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
                                     [](auto&&) { return true; });
    }

    void network_tick(float dt) {
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

            send_updated_state();
            host_send_player();
        };
        if (desired_role & s_Host) network_host(dt);

        auto network_client = [&](float) {
            if (!client.is_connecting_or_connected()) {
                return;
            }
            client_send_player();
            client.consume_events();
        };
        if (desired_role & s_Client) network_client(dt);
    }

    void send_updated_state() {
        ClientPacket player({
            .client_id = my_client_id,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo(
                {.host_menu_state = Menu::get().state}),
        });

        Buffer buffer;
        auto size = bitsery::quickSerialization(OutputAdapter{buffer}, player);

        server.send_packet_to_all_if(
            0, reinterpret_cast<const enet_uint8*>(buffer.data()), size,
            ENET_PACKET_FLAG_RELIABLE, [](const ThinClient&) { return true; });
    }
};

}  // namespace network
