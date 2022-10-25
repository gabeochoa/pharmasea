

#include "network.h"

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define ENET_IMPLEMENTATION
#include <enet.h>
//
#include <enetpp.h>
//
#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/ext/std_tuple.h>
#include <bitsery/ext/std_variant.h>
#include <bitsery/serializer.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/ostream.h>
// this is needed for wstring printing
#include <fmt/xchar.h>

#include <cstdint>
#include <cstring>
#include <variant>
#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif
//
#include "../random.h"
#include "../util.h"

namespace network {

using Buffer = std::vector<unsigned char>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

struct BaseInternal {
    std::map<int, ClientPacket::PlayerInfo> clients_to_process;
};

typedef std::variant<ClientPacket::PingInfo, ClientPacket::PlayerJoinInfo,
                     ClientPacket::GameStateInfo, ClientPacket::WorldInfo,
                     ClientPacket::PlayerInfo>
    Msg;

std::ostream& operator<<(std::ostream& os, const Msg& msgtype) {
    os << std::visit(
        util::overloaded{
            [&](ClientPacket::PingInfo) { return std::string("ping"); },
            [&](ClientPacket::PlayerJoinInfo info) {
                return fmt::format("PlayerJoinInfo( is_you: {}, id: {})",
                                   info.is_you, info.client_id);
            },
            [&](ClientPacket::GameStateInfo info) {
                return fmt::format("GameStateInfo( state: {} )",
                                   info.host_menu_state);
            },
            [&](ClientPacket::WorldInfo) { return std::string("worldinfo"); },
            [&](ClientPacket::PlayerInfo info) {
                return fmt::format(
                    "PlayerInfo( id{} name{} pos({}, {}, {}), facing {})",
                    info.client_id, info.name, info.location[0],
                    info.location[1], info.location[2], info.facing_direction);
            },
            [&](auto) { return std::string(" -- invalid operator<< --"); }},
        msgtype);
    return os;
}

std::ostream& operator<<(std::ostream& os,
                         const ClientPacket::MsgType& msgtype) {
    switch (msgtype) {
        case ClientPacket::Ping:
            os << "Ping";
            break;
        case ClientPacket::GameState:
            os << "GameState";
            break;
        case ClientPacket::World:
            os << "WorldInfo";
            break;
        case ClientPacket::PlayerLocation:
            os << "PlayerLocation";
            break;
        case ClientPacket::PlayerJoin:
            os << "PlayerJoinInfo";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const ClientPacket& packet) {
    os << "Packet(" << packet.client_id << ": " << packet.msg_type << " "
       << packet.msg << ")" << std::endl;
    return os;
}

template<typename S>
void serialize(S& s, ClientPacket& packet) {
    s.value4b(packet.client_id);
    s.value4b(packet.msg_type);
    s.ext(packet.msg, bitsery::ext::StdVariant{
                          [](S&, ClientPacket::PingInfo&) {},
                          [](S& s, ClientPacket::PlayerJoinInfo& info) {
                              s.value1b(info.is_you);
                              s.value4b(info.client_id);
                          },
                          [](S& s, ClientPacket::GameStateInfo& info) {
                              s.value4b(info.host_menu_state);
                          },
                          [](S&, ClientPacket::WorldInfo&) {},
                          [](S& s, ClientPacket::PlayerInfo& info) {
                              // From Join Info
                              s.value1b(info.is_you);
                              s.value4b(info.client_id);
                              // end
                              s.text1b(info.name, MAX_NAME_LENGTH);
                              s.value4b(info.location[0]);
                              s.value4b(info.location[1]);
                              s.value4b(info.location[2]);
                              s.value4b(info.facing_direction);
                          },
                      });
}

Info::Info() {
    enetpp::global_state::get().initialize();
    server = new enetpp::server<ThinClient>();
    client = new enetpp::client();
}

Info::~Info() {
    close_active_roles();
    enetpp::global_state::get().deinitialize();
    delete server;
    delete client;
}

void Info::close_active_roles() {
    if (desired_role & s_Client) client->disconnect();
    if (desired_role & s_Host) server->stop_listening();
}

void Info::fwd_packet_to_other_clients(int source_id, ClientPacket packet) {
    Buffer buffer;
    bitsery::quickSerialization(OutputAdapter{buffer}, packet);
    server->send_packet_to_all_if(0, buffer.data(), buffer.size(),
                                  ENET_PACKET_FLAG_RELIABLE,
                                  [&](const ThinClient& destination) {
                                      return destination.get_id() != source_id;
                                  });
}

void Info::fwd_packet_to_source(int source_id, ClientPacket packet) {
    Buffer buffer;
    bitsery::quickSerialization(OutputAdapter{buffer}, packet);
    server->send_packet_to_all_if(0, buffer.data(), buffer.size(),
                                  ENET_PACKET_FLAG_RELIABLE,
                                  [&](const ThinClient& destination) {
                                      return destination.get_id() == source_id;
                                  });
}

void Info::process_client_packet_msg(int client_id, ClientPacket packet) {
    auto player_join = [&](ClientPacket::PlayerJoinInfo& player_join_info) {
        // NOTE: for this message type, the packet.client_id,
        // isnt yet filled in correctly and is likely -1
        // so we use the client_id in the message instead
        if (player_join_info.is_you) {
            // im the player
            my_client_id = player_join_info.client_id;
            return;
        }

        add_new_player_cb(client_id, packet.client_id);
        return;
    };

    auto player_location = [&]() {
        ClientPacket::PlayerInfo player_info =
            std::get<ClientPacket::PlayerInfo>(packet.msg);
        int id = packet.client_id;
        if (is_host()) id = client_id;

        add_new_player_cb(client_id, id);
        update_remote_player_cb(client_id, player_info.name,
                                player_info.location,
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
                    ClientPacket({.client_id = my_client_id,
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

void Info::start_server() {
    auto init_client_func = [](ThinClient& thin_client, const char* ip) {
        auto h1 = std::hash<std::string>{}(std::string(ip)) + randIn(0, 1000);
        thin_client._uid = (int) h1;
        std::cout << "last client id was: " << h1 << std::endl;
    };
    my_client_id = -2;

    server->set_trace_handler([](const std::string& line) {
        std::cout << "server: " << line << std::endl;
    });

    server->start_listening(
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
        remove_player_cb(client_id);
    };
    auto on_client_data_received = [&](ThinClient& source_client,
                                       const enet_uint8* data,
                                       size_t data_size) {
        // local processing
        Buffer buffer;
        for (std::size_t i = 0; i < data_size; i++) buffer.push_back(data[i]);
        ClientPacket packet;
        bitsery::quickDeserialization<InputAdapter>({buffer.begin(), data_size},
                                                    packet);
        process_client_packet_msg(source_client.get_id(), packet);
    };

    server->register_callbacks("server", on_client_connected,
                               on_client_disconnected, on_client_data_received);
}

void Info::start_client() {
    client->set_trace_handler([](const std::string& line) {
        std::cout << "client: " << line << std::endl;
    });

    client->connect(
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
        bitsery::quickSerialization(OutputAdapter{buffer}, connected_packet);
        client->send_packet(0, buffer.data(), buffer.size(),
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
        for (std::size_t i = 0; i < data_size; i++) buffer.push_back(data[i]);
        ClientPacket packet;
        bitsery::quickDeserialization<InputAdapter>({buffer.begin(), data_size},
                                                    packet);
        process_client_packet_msg(0, packet);
    };

    client->register_callbacks("client", on_connected, on_disconnected,
                               on_data_received);
}

Buffer Info::get_player_packet() {
    auto player = player_packet_info_cb(my_client_id);
    Buffer buffer;
    bitsery::quickSerialization(OutputAdapter{buffer}, player);
    return buffer;
}

void Info::client_send_player() {
    Buffer buffer = get_player_packet();
    client->send_packet(0, buffer.data(), buffer.size(),
                        ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
}

void Info::host_send_player() {
    Buffer buffer = get_player_packet();
    server->send_packet_to_all_if(0, buffer.data(), buffer.size(),
                                  ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT,
                                  [](auto&&) { return true; });
}

void Info::network_tick(float dt) {
    auto network_host = [&](float) {
        if (!server->is_listening()) return;
        // send stuff to specific client where uid=123
        std::string packet = "data_to_send";
        assert(sizeof(char) == sizeof(enet_uint8));
        // server->send_packet_to(123, 0, &data_to_send, 1,
        // ENET_PACKET_FLAG_RELIABLE);
        //
        // send stuff to all clients (with optional predicate filter)
        server->send_packet_to_all_if(
            0, reinterpret_cast<const enet_uint8*>(packet.data()),
            packet.length(), ENET_PACKET_FLAG_RELIABLE,
            [](const ThinClient&) { return true; });

        server->consume_events();

        // get access to all connected clients
        // for (auto c : server->get_connected_clients()) { }

        send_updated_state();
        host_send_player();
    };
    if (desired_role & s_Host) network_host(dt);

    auto network_client = [&](float) {
        if (!client->is_connecting_or_connected()) {
            return;
        }
        client_send_player();
        client->consume_events();
    };
    if (desired_role & s_Client) network_client(dt);
}

void Info::send_updated_state() {
    ClientPacket player({
        .client_id = my_client_id,
        .msg_type = ClientPacket::MsgType::GameState,
        .msg =
            ClientPacket::GameStateInfo({.host_menu_state = Menu::get().state}),
    });

    Buffer buffer;
    auto size = bitsery::quickSerialization(OutputAdapter{buffer}, player);

    server->send_packet_to_all_if(
        0, reinterpret_cast<const enet_uint8*>(buffer.data()), size,
        ENET_PACKET_FLAG_RELIABLE, [](const ThinClient&) { return true; });
}
};  // namespace network
