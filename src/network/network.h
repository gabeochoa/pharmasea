

#pragma once

#include "../globals.h"
//
#include "../entities.h"
#include "../player.h"
#include "../remote_player.h"
#include "../settings.h"
#include "../singleton.h"
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

SINGLETON_FWD(Info)
struct Info {
    SINGLETON(Info)

    network::LobbyState network_lobby_state;
    bool processed_updated_lobby_state = true;

    int my_client_id = 0;
    bool username_set = false;
    std::string host_ip_address = "127.0.0.1";
    bool ip_set = false;
    float client_next_tick_reset = 0.02f;
    float client_next_tick = 0.0f;
    std::map<int, std::shared_ptr<RemotePlayer>> remote_players;

    enum Role {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    std::shared_ptr<Server> server_p;
    std::shared_ptr<Client> client_p;

    bool is_host() { return desired_role & s_Host; }
    bool is_client() { return desired_role & s_Client; }
    bool has_role() { return is_host() || is_client(); }
    bool has_set_ip() { return is_host() || (is_client() && ip_set); }
    void lock_in_ip() {
        ip_set = true;
        client_p->set_address(host_ip_address);
        client_p->startup();
    }

    Info() {}

    ~Info() {
        server_p.reset();
        client_p.reset();
        //
        desired_role = s_None;
        ip_set = false;
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

    void set_role_to_host() {
        desired_role = s_Host;
        server_p.reset(new Server(DEFAULT_PORT));
        server_p->set_process_message(
            std::bind(&Info::server_process_message_string, this,
                      std::placeholders::_1, std::placeholders::_2));
        server_p->startup();

        //
        client_p.reset(new Client(true));
        client_p->set_process_message(std::bind(
            &Info::client_process_message_string, this, std::placeholders::_1));
        host_ip_address = "127.0.0.1";
        lock_in_ip();
    }

    void set_role_to_client() {
        desired_role = s_Client;
        client_p.reset(new Client());
        client_p->set_process_message(std::bind(
            &Info::client_process_message_string, this, std::placeholders::_1));
    }

    void tick(float dt) {
        this->client_next_tick = this->client_next_tick - dt;

        auto _tick_client = [&](float) {
            client_p->run();
            if (this->client_next_tick > 0) {
                return;
            }
            this->client_next_tick = client_next_tick_reset;
            if (my_client_id > 0) {
                auto player = get_player_packet(my_client_id);
                client_p->send_packet_to_server(player);
            }
        };

        if (desired_role & s_Host) {
            server_p->run();
            _tick_client(dt);
        }

        if (desired_role & s_Client) {
            _tick_client(dt);
        }
    }

    void send_updated_state(network::LobbyState state) {
        ClientPacket player({
            .client_id = my_client_id,
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

    void client_process_message_string(std::string msg) {
        // std::cout << "client process message string " << std::endl;
        auto add_new_player = [&](int client_id) {
            if (remote_players.contains(client_id)) {
                std::cout << fmt::format("Why are we trying to add {}",
                                         client_id)
                          << std::endl;
            };

            remote_players[client_id] = std::make_shared<RemotePlayer>();
            auto rp = remote_players[client_id];
            rp->client_id = client_id;
            EntityHelper::addEntity(remote_players[client_id]);
            std::cout << fmt::format("Adding a player {}", client_id)
                      << std::endl;
        };

        auto remove_player = [&](int client_id) {
            auto rp = remote_players[client_id];
            if (!rp)
                std::cout << fmt::format("doesnt exist but should {}",
                                         client_id)
                          << std::endl;
            rp->cleanup = true;
            remote_players.erase(client_id);
        };

        auto update_remote_player =
            [&](int client_id, std::string name, float* location, int facing) {
                if (!remote_players.contains(client_id)) {
                    std::cout
                        << fmt::format("doesnt exist but should {}", client_id)
                        << std::endl;
                    add_new_player(client_id);
                }
                auto rp = remote_players[client_id];
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
                    my_client_id = info.client_id;
                    log(fmt::format("my id is {}", my_client_id));
                }

                for (auto id : info.all_clients) {
                    if (info.is_you && id == info.client_id) continue;
                    // otherwise someone just joined and we have to deal with
                    // them
                    add_new_player(id);
                }

            } break;
            case ClientPacket::MsgType::GameState: {
                ClientPacket::GameStateInfo info =
                    std::get<ClientPacket::GameStateInfo>(packet.msg);
                network_lobby_state = info.host_menu_state;
                processed_updated_lobby_state = false;
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

            case ClientPacket::MsgType::PlayerJoin: {
                // We dont need this
                // ClientPacket::PlayerJoinInfo info =
                // std::get<ClientPacket::PlayerJoinInfo>(packet.msg);

                std::vector<int> ids;
                for (auto& c : server_p->clients) {
                    ids.push_back(c.second.client_id);
                }
                // Since we are the host, we can use the Client_t to figure out
                // the id / name
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
