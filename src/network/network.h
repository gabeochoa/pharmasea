

#pragma once

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
    int my_client_id;
    std::string username = "default username";
    bool username_set = true;
    // TODO eventually support copy/paste
    std::string host_ip_address = "127.0.0.1";
    bool ip_set = false;

    enum State {
        s_None = 1 << 0,
        s_Host = 1 << 1,
        s_Client = 1 << 2,
    } desired_role = s_None;

    std::shared_ptr<Server> server_p;
    std::shared_ptr<Client> client_p;

    std::function<network::ClientPacket(int)> player_packet_info_cb;
    std::function<void(int)> add_new_player_cb;
    std::function<void(int)> remove_player_cb;
    std::function<void(int, std::string, float[3], int)>
        update_remote_player_cb;

    bool is_host() { return desired_role & s_Host; }
    bool is_client() { return desired_role & s_Client; }
    bool has_role() { return is_host() || is_client(); }
    bool has_set_ip() { return is_host() || (is_client() && ip_set); }
    void lock_in_ip() {
        ip_set = true;
        client_p->set_address(host_ip_address);
        client_p->startup();
    }

    void init_connections() {
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

    void shutdown_connections() {
#ifdef BUILD_WITHOUT_STEAM
        GameNetworkingSockets_Kill();
#endif
    }

    void set_role_to_host() {
        desired_role = s_Host;
        init_connections();
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
        init_connections();
        client_p.reset(new Client());
        client_p->set_process_message(std::bind(
            &Info::client_process_message_string, this, std::placeholders::_1));
    }

    void set_role_to_none() {
        desired_role = s_None;
        ip_set = false;
        server_p->teardown();
        server_p.reset();
        client_p.reset();
        shutdown_connections();
    }

    void tick(float) {
        if (desired_role & s_Host) {
            server_p->run();
            client_p->run();
        }

        if (desired_role & s_Client) {
            client_p->run();
        }
    }

    void register_new_player_cb(std::function<void(int)> cb) {
        add_new_player_cb = cb;
    }

    void register_remove_player_cb(std::function<void(int)> cb) {
        remove_player_cb = cb;
    }

    void register_update_player_cb(
        std::function<void(int, std::string, float[3], int)> cb) {
        update_remote_player_cb = cb;
    }

    void register_player_packet_cb(
        std::function<network::ClientPacket(int)> cb) {
        player_packet_info_cb = cb;
    }

    void send_updated_state() {
        ClientPacket player({
            .client_id = my_client_id,
            .msg_type = ClientPacket::MsgType::GameState,
            .msg = ClientPacket::GameStateInfo(
                {.host_menu_state = Menu::get().state}),
        });
        server_p->send_client_packet_to_all(player);
    }

    void client_process_message_string(std::string msg) {
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
                    return;
                }
                // otherwise someone just joined and we have to deal with them
                add_new_player_cb(info.client_id);

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

                // Since we are the host, we can use the Client_t to figure out
                // the id / name
                server_p->send_client_packet_to_all(
                    ClientPacket(
                        {.client_id = SERVER_CLIENT_ID,
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
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
                log(fmt::format("Server: {} not handled yet: {} ",
                                packet.msg_type, msg));
                break;
        }
    }
};

}  // namespace network
