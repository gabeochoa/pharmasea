

#pragma once

#include "../globals.h"
//
#include "../entities.h"
#include "../player.h"
#include "../remote_player.h"
#include "../settings.h"
//
#include "shared.h"
//
#include "client.h"
#include "server.h"

namespace network {

static void log(std::string msg) { std::cout << msg << std::endl; }
// std::this_thread::sleep_for(std::chrono::milliseconds(500));

static SteamNetworkingMicroseconds START_TIME;

static void log_debug(ESteamNetworkingSocketsDebugOutputType eType,
                      const char* pszMsg) {
    SteamNetworkingMicroseconds time =
        SteamNetworkingUtils()->GetLocalTimestamp() - START_TIME;
    printf("%10.6f %s\n", time * 1e-6, pszMsg);
    fflush(stdout);
    if (eType == k_ESteamNetworkingSocketsDebugOutputType_Bug) {
        fflush(stdout);
        fflush(stderr);
        exit(1);
    }
}

struct Info {
    struct ConnectionInfo {
        std::string host_ip_address = "127.0.0.1";
        bool ip_set = false;
    } conn_info;

    struct ClientData {
        int id = 0;
        bool username_set = false;
        std::map<int, std::shared_ptr<RemotePlayer>> remote_players;

        float next_tick_reset = 0.02f;
        float next_tick = 0.0f;
    } client_data;

    struct RoleInfo {
        enum Role {
            s_None = 1 << 0,
            s_Host = 1 << 1,
            s_Client = 1 << 2,
        } desired_role = s_None;

        bool is_host() { return desired_role & s_Host; }
        bool is_client() { return desired_role & s_Client; }
        bool has_role() { return is_host() || is_client(); }

    } role_info;

    bool is_host() { return role_info.is_host(); }
    bool has_role() { return role_info.has_role(); }
    bool username_set() { return client_data.username_set; }

    bool has_set_ip() {
        return role_info.is_host() ||
               (role_info.is_client() && conn_info.ip_set);
    }

    void lock_in_ip() {
        conn_info.ip_set = true;
        client_p->set_address(conn_info.host_ip_address);
        client_p->startup();
    }

    std::string& host_ip_address() { return conn_info.host_ip_address; }

    void lock_in_username() { client_data.username_set = true; }
    void unlock_username() { client_data.username_set = false; }

    void set_role_to_host() {
        role_info.desired_role = RoleInfo::Role::s_Host;
        server_p.reset(new Server(DEFAULT_PORT));
        server_p->set_process_message(
            std::bind(&Info::server_process_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->startup();

        //
        client_p.reset(new Client(true));
        client_p->set_process_message(std::bind(
            &Info::client_process_message_string, this, std::placeholders::_1));
        conn_info.host_ip_address = "127.0.0.1";
        lock_in_ip();
    }

    void set_role_to_client() {
        role_info.desired_role = RoleInfo::Role::s_Client;
        client_p.reset(new Client());
        client_p->set_process_message(std::bind(
            &Info::client_process_message_string, this, std::placeholders::_1));
    }

    //

    std::shared_ptr<Server> server_p;
    std::shared_ptr<Client> client_p;

    Info() {}

    ~Info() {
        server_p.reset();
        client_p.reset();
        //
        role_info.desired_role = RoleInfo::Role::s_None;
        conn_info.ip_set = false;
    }

    static void init_connections() {
#ifdef BUILD_WITHOUT_STEAM
        SteamDatagramErrMsg errMsg;
        if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
            log(fmt::format("GameNetworkingSockets init failed {}", errMsg));
        }
#endif
        START_TIME = SteamNetworkingUtils()->GetLocalTimestamp();
        SteamNetworkingUtils()->SetDebugOutputFunction(
            k_ESteamNetworkingSocketsDebugOutputType_Msg, log_debug);
    }

    static void shutdown_connections() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    }

    void tick(float dt) {
        client_data.next_tick = client_data.next_tick - dt;

        auto _tick_client = [&](float) {
            client_p->run();
            if (client_data.next_tick > 0) {
                return;
            }
            client_data.next_tick = client_data.next_tick_reset;
            if (client_data.id > 0) {
                // auto player = get_player_packet(my_client_id);
                // client_p->send_packet_to_server(player);

                // auto playerinput = get_player_input_packet(client_data.id);
                // client_p->send_packet_to_server(playerinput);
            }
        };

        if (role_info.is_host()) {
            server_p->run();
            _tick_client(dt);
        }

        if (role_info.is_client()) {
            _tick_client(dt);
        }
    }

    void send_updated_state(Menu::State state) {
        ClientPacket player({
            .client_id = client_data.id,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo({.host_menu_state = state}),
        });
        server_p->send_client_packet_to_all(player);
    }

    ClientPacket get_player_packet(int my_id) {
        Player me = GLOBALS.get<Player>("player");
        ClientPacket player({
            .channel = Channel::UNRELIABLE_NO_DELAY,
            .client_id = my_id,
            .msg_type = network::ClientPacket::MsgType::PlayerLocation,
            .msg = network::ClientPacket::PlayerInfo({
                .facing_direction = static_cast<int>(me.face_direction),
                .location =
                    {
                        me.position.x,
                        me.position.y,
                        me.position.z,
                    },
                .name = Settings::get().data.username,
            }),
        });
        return player;
    }

    ClientPacket get_player_input_packet(int my_id) {
        Player* me = GLOBALS.get_ptr<Player>("player");
        ClientPacket player({
            .channel = Channel::UNRELIABLE_NO_DELAY,
            .client_id = my_id,
            .msg_type = network::ClientPacket::MsgType::PlayerControl,
            .msg = network::ClientPacket::PlayerControlInfo({
                .inputs = me->inputs,
            }),
        });
        me->inputs.clear();
        return player;
    }

    void client_process_message_string(std::string msg) {
        auto add_new_player = [&](int client_id) {
            if (client_data.remote_players.contains(client_id)) {
                std::cout << fmt::format("Why are we trying to add {}",
                                         client_id)
                          << std::endl;
            };

            client_data.remote_players[client_id] =
                std::make_shared<RemotePlayer>();
            auto rp = client_data.remote_players[client_id];
            rp->client_id = client_id;
            EntityHelper::addEntity(client_data.remote_players[client_id]);
            std::cout << fmt::format("Adding a player {}", client_id)
                      << std::endl;
        };

        auto remove_player = [&](int client_id) {
            auto rp = client_data.remote_players[client_id];
            if (!rp)
                std::cout << fmt::format("doesnt exist but should {}",
                                         client_id)
                          << std::endl;
            rp->cleanup = true;
            client_data.remote_players.erase(client_id);
        };

        auto update_remote_player =
            [&](int client_id, std::string name, float* location, int facing) {
                if (!client_data.remote_players.contains(client_id)) {
                    std::cout
                        << fmt::format("doesnt exist but should {}", client_id)
                        << std::endl;
                    add_new_player(client_id);
                }
                auto rp = client_data.remote_players[client_id];
                if (!rp) return;
                rp->update_remotely(name, location, facing);
            };

        ClientPacket packet;
        bitsery::quickDeserialization<InputAdapter>({msg.begin(), msg.size()},
                                                    packet);

        switch (packet.msg_type) {
            case ClientPacket::MsgType::Announcement: {
                ClientPacket::AnnouncementInfo info =
                    std::get<ClientPacket::AnnouncementInfo>(packet.msg);
                log(fmt::format("Announcement: {}", info.message));
            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                ClientPacket::PlayerJoinInfo info =
                    std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

                if (info.is_you) {
                    // We are the person that joined,
                    client_data.id = info.client_id;
                    log(fmt::format("my id is {}", client_data.id));
                }

                for (auto id : info.all_clients) {
                    if (info.is_you && id == info.client_id) continue;
                    // otherwise someone just joined and we have to deal
                    // with them
                    add_new_player(id);
                }

            } break;
            case ClientPacket::MsgType::GameState: {
                ClientPacket::GameStateInfo info =
                    std::get<ClientPacket::GameStateInfo>(packet.msg);
                Menu::get().state = info.host_menu_state;
            } break;
            case ClientPacket::MsgType::PlayerLocation: {
                ClientPacket::PlayerInfo info =
                    std::get<ClientPacket::PlayerInfo>(packet.msg);
                update_remote_player(packet.client_id, info.name, info.location,
                                     info.facing_direction);
            } break;

            default:
                log(fmt::format("Client: {} not handled yet: {} ",
                                packet.msg_type, msg));
                break;
        }
    }

    void server_process_message_string(const Client_t& incoming_client,
                                       std::string msg) {
        ClientPacket packet;
        bitsery::quickDeserialization<InputAdapter>({msg.begin(), msg.size()},
                                                    packet);
        switch (packet.msg_type) {
            case ClientPacket::MsgType::Announcement: {
                // TODO send announcements to all clients
                ClientPacket::AnnouncementInfo info =
                    std::get<ClientPacket::AnnouncementInfo>(packet.msg);
            } break;

            case ClientPacket::MsgType::PlayerControl: {
                ClientPacket::PlayerControlInfo info =
                    std::get<ClientPacket::PlayerControlInfo>(packet.msg);

                auto remote_player =
                    client_data.remote_players[incoming_client.client_id];
                if (!remote_player) return;
                auto updated_position =
                    remote_player->get_position_after_input(info.inputs);

                ClientPacket player_updated({
                    .channel = Channel::UNRELIABLE_NO_DELAY,
                    .client_id = incoming_client.client_id,
                    .msg_type = network::ClientPacket::MsgType::PlayerLocation,
                    .msg = network::ClientPacket::PlayerInfo({
                        .facing_direction =
                            static_cast<int>(remote_player->face_direction),
                        .location =
                            {
                                updated_position.x,
                                updated_position.y,
                                updated_position.z,
                            },
                        .name = remote_player->name,
                    }),
                });

                server_p->send_client_packet_to_all(player_updated);

            } break;
            case ClientPacket::MsgType::PlayerJoin: {
                // We dont need this
                // ClientPacket::PlayerJoinInfo info =
                // std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

                std::vector<int> ids;
                for (auto& c : server_p->clients) {
                    ids.push_back(c.second.client_id);
                }
                // Since we are the host, we can use the Client_t to figure
                // out the id / name
                server_p->send_client_packet_to_all(
                    ClientPacket(
                        {.client_id = SERVER_CLIENT_ID,
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = false,
                         })}),
                    // ignore the person who sent it to us
                    [&](Client_t& client) {
                        return client.client_id == incoming_client.client_id;
                    });

                server_p->send_client_packet_to_all(
                    ClientPacket(
                        {.client_id = SERVER_CLIENT_ID,
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = true,
                         })}),
                    // ignore everyone except the one that sent to us
                    [&](Client_t& client) {
                        return client.client_id != incoming_client.client_id;
                    });
            } break;
            default:
                server_p->send_client_packet_to_all(
                    packet, [&](Client_t& client) {
                        return client.client_id == incoming_client.client_id;
                    });
                // log(fmt::format("Server: {} not handled yet ",
                // packet.msg_type));
                break;
        }
    }
};

}  // namespace network
