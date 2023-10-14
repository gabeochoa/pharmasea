#include "server.h"

#include <thread>

#include "shared.h"

namespace network {

static std::shared_ptr<Server> g_server;

// TODO once clang supports jthread replace with jthread and remove "running
// = true" to use stop_token
std::thread Server::start(int port) {
    // TODO makeshared doesnt work here
    g_server.reset(new Server(port));
    g_server->running = true;
    return std::thread(std::bind(&Server::run, g_server.get()));
}

void Server::queue_packet(const ClientPacket& p) {
    g_server->incoming_packet_queue.push_back(p);
}
void Server::forward_packet(const ClientPacket& p) {
    g_server->packet_queue.push_back(p);
}

void Server::play_sound(vec2 position, const std::string& sound) {
    Server::forward_packet(  //
        ClientPacket{
            //
            .client_id = SERVER_CLIENT_ID,
            .msg_type = ClientPacket::MsgType::PlaySound,
            .msg =
                ClientPacket::PlaySoundInfo{
                    //
                    .location = {position.x, position.y},
                    .sound = sound
                    //
                }
            //
        }  //
    );
}

void Server::stop() { g_server->running = false; }

void Server::send_map_state() {
    pharmacy_map->grab_things();

    ClientPacket map_packet{
        .channel = Channel::RELIABLE,
        .client_id = SERVER_CLIENT_ID,
        .msg_type = network::ClientPacket::MsgType::Map,
        .msg =
            network::ClientPacket::MapInfo{
                .map = *pharmacy_map,
            },
    };

    send_client_packet_to_all(map_packet);
}

void Server::send_player_rare_data() {
    for (const auto& player : players) {
        ClientPacket player_rare_updated{
            .channel = Channel::UNRELIABLE,
            // Pretend this came from the other client
            .client_id = player.first,
            .msg_type = network::ClientPacket::MsgType::PlayerRare,
            .msg =
                network::ClientPacket::PlayerRareInfo{
                    .client_id = player.first,
                    .model_index = player.second->get<UsesCharacterModel>()
                                       .index_server_only(),
                    .last_ping = player.second->get<HasClientID>().ping(),
                },
        };
        send_client_packet_to_all(player_rare_updated);
    }
}

void Server::run() {
    TRACY_ZONE_SCOPED;
    thread_id = std::this_thread::get_id();
    GLOBALS.set("server_thread_id", &thread_id);

    constexpr float desiredFrameRate = 120.0f;
    constexpr std::chrono::duration<float> fixedTimeStep(1.0f /
                                                         desiredFrameRate);

    // Initialize time variables
    auto previousTime = std::chrono::high_resolution_clock::now();
    auto currentTime = previousTime;

    while (running) {
        currentTime = std::chrono::high_resolution_clock::now();
        float duration =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                currentTime - previousTime)
                .count();

        if (duration >= fixedTimeStep.count()) {
            tick(fixedTimeStep.count());

            // Update previous time to keep track of the next tick
            previousTime = currentTime;
        }

        // Sleep to control the frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// float fps_timer = 1.f;
// int frames = 0;
//
// void fps(float dt) {
// fps_timer -= dt;
// frames++;
// if (fps_timer <= 0) {
// fps_timer = 1.f;
// log_info("{} {} imq{} fwq{}", dt, frames,
// incoming_message_queue.size(), packet_queue.size());
// frames = 0;
// }
// }

void Server::tick(float dt) {
    // fps(dt);

    TRACY_ZONE_SCOPED;
    server_p->run();

    // network
    process_incoming_messages();
    process_incoming_packets();
    process_packet_forwarding();

    // game
    process_map_update(dt);

    // TODO sending updates takes ~15x the time of running the game
    // gotta look into what to do here to reduce the amount of info we are
    // sending
    //  / if theres networking settings we can change

    // send updates back
    process_player_rare_tick(dt);
    process_map_sync(dt);

    TRACY_FRAME_MARK("server::tick");
}

void Server::process_incoming_messages() {
    // Check to see if we have any new packets to process
    while (!incoming_message_queue.empty()) {
        TRACY_ZONE_NAMED(tracy_server_process, "packets to process", true);
        log_trace("Incoming Messages {}", incoming_message_queue.size());
        server_process_message_string(incoming_message_queue.front());
        incoming_message_queue.pop_front();
    }
}

void Server::process_incoming_packets() {
    // Check to see if we have any new packets to process
    while (!incoming_packet_queue.empty()) {
        log_trace("Incoming Packets {}", incoming_message_queue.size());
        server_process_packet({.client_id = SERVER_CLIENT_ID},
                              incoming_packet_queue.front());
        incoming_packet_queue.pop_front();
    }
}

void Server::process_packet_forwarding() {
    // TODO move one of these while loops, probably the one belowVVV to a
    // different thread
    // This will allow us to empty the steam queue faster, and give us more
    // control over which messages to drop

    // Check to see if we have any packets to send off
    while (!packet_queue.empty()) {
        TRACY_ZONE_NAMED(tracy_server_fwd, "packets to fwd", true);
        log_trace("Packets to FWD {}", packet_queue.size());
        ClientPacket& p = packet_queue.front();

        switch (p.msg_type) {
            case ClientPacket::MsgType::GameState: {
                ClientPacket::GameStateInfo info =
                    std::get<ClientPacket::GameStateInfo>(p.msg);
                // TODO probably dont need this
                current_menu_state = info.host_menu_state;
                current_game_state = info.host_game_state;
            } break;
            default:
                break;
        }

        //
        send_client_packet_to_all(p);
        packet_queue.pop_front();
    }
}

void Server::process_map_update(float dt) {
    // No need to do anything if we are still in the menu
    if (!MenuState::s_is_game(current_menu_state)) return;

    // TODO right now we have the run update on all the server players
    // this kinda makes sense but most of the game doesnt need this.
    //
    // the big ones are held item/furniture position updates
    // without this the position doesnt change until you drop it
    //
    // TODO idk the perf implications of this (map->vec)

    std::vector<std::shared_ptr<Entity>> temp_players;
    for (auto it = players.begin(); it != players.end(); ++it) {
        temp_players.push_back(it->second);
    }

    pharmacy_map->_onUpdate(temp_players, dt);

    TRACY_ZONE(tracy_server_gametick);
}

void Server::process_map_sync(float dt) {
    next_map_tick -= dt;
    if (next_map_tick <= 0) {
        send_map_state();
        next_map_tick += next_map_tick_reset;
    }
}

void Server::process_player_rare_tick(float dt) {
    next_player_rare_tick -= dt;
    if (next_player_rare_tick <= 0) {
        // TODO decide if this needs to be more / less often
        send_player_rare_data();
        next_player_rare_tick = next_player_rare_tick_reset;
    }
}

void Server::process_announcement_packet(const internal::Client_t&,
                                         const ClientPacket& packet) {
    [[maybe_unused]] const ClientPacket::AnnouncementInfo info =
        std::get<ClientPacket::AnnouncementInfo>(packet.msg);
    send_client_packet_to_all(packet);
}

void Server::process_player_control_packet(
    const internal::Client_t& incoming_client, const ClientPacket& packet) {
    const ClientPacket::PlayerControlInfo info =
        std::get<ClientPacket::PlayerControlInfo>(packet.msg);

    auto player = players[packet.client_id];

    if (!player) return;

    // vvv i think we already did this but not 100% sure
    // TODO interpolate our old position and new position so its smoother
    SystemManager::get().process_inputs(Entities{player}, info.inputs);
    auto updated_position = player->get<Transform>().pos();

    // TODO if the position and face direction didnt change
    //      then we can early return
    //
    // NOTE: i saw issues where == between vec3 was returning
    //      on every call because (mvt * dt) < epsilon
    //

    send_player_location_packet(incoming_client.client_id, updated_position,
                                player->get<Transform>().facing,
                                player->get<HasName>().name());
}

void Server::send_player_location_packet(int client_id, const vec3& pos,
                                         float facing,
                                         const std::string& name) {
    ClientPacket player_updated{
        .channel = Channel::UNRELIABLE,
        .client_id = client_id,
        .msg_type = network::ClientPacket::MsgType::PlayerLocation,
        .msg = network::ClientPacket::PlayerInfo{.facing = facing,
                                                 .location =
                                                     {
                                                         pos.x,
                                                         pos.y,
                                                         pos.z,
                                                     },
                                                 .username = name},
    };

    send_client_packet_to_all(player_updated);
}

void Server::send_announcement(HSteamNetConnection conn, const std::string& msg,
                               internal::InternalServerAnnouncement type) {
    AnnouncementType announcementInfo;

    switch (type) {
        case internal::InternalServerAnnouncement::Info:
            announcementInfo = AnnouncementType::Message;
            break;
        case internal::InternalServerAnnouncement::Warn:
            announcementInfo = AnnouncementType::Warning;
            break;
        case internal::InternalServerAnnouncement::Error:
            announcementInfo = AnnouncementType::Error;
            break;
    }

    ClientPacket announce_packet{
        .client_id = SERVER_CLIENT_ID,
        .msg_type = ClientPacket::MsgType::Announcement,
        .msg = ClientPacket::AnnouncementInfo{.message = msg,
                                              .type = announcementInfo}};

    send_client_packet_to_client(conn, announce_packet);
}

void Server::process_player_leave(int client_id) {
    log_info("processing player leave for {}", client_id);
    auto player_match = players.find(client_id);
    if (player_match == players.end()) {
        log_warn("We dont have a matching player for {}", client_id);
        return;
    }
    // TODO We might have to force them to drop everything or something?
    players.erase(player_match);

    std::vector<int> ids;
    for (auto& c : server_p->clients) {
        ids.push_back(c.second.client_id);
    }
    // Since we are the host, we can use the internal::Client_t to figure
    // out the id / name
    send_client_packet_to_all(
        ClientPacket{.client_id = SERVER_CLIENT_ID,
                     .msg_type = ClientPacket::MsgType::PlayerLeave,
                     .msg =
                         ClientPacket::PlayerLeaveInfo{
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = client_id,
                         }},
        // ignore the person who sent it to us since they disconn
        [&](internal::Client_t& client) {
            return client.client_id == client_id;
        });
}

void Server::process_player_leave_packet(
    const internal::Client_t& incoming_client, const ClientPacket&) {
    log_info("processing player leave packet for {}",
             incoming_client.client_id);
    // ClientPacket::PlayerLeaveInfo info =
    // std::get<ClientPacket::PlayerLeaveInfo>(orig_packet.msg);
    process_player_leave(incoming_client.client_id);
}

void Server::process_player_join_packet(
    const internal::Client_t& incoming_client,
    const ClientPacket& orig_packet) {
    ClientPacket::PlayerJoinInfo info =
        std::get<ClientPacket::PlayerJoinInfo>(orig_packet.msg);

    if (info.hashed_version != HASHED_VERSION) {
        // TODO send error message announcement
        log_warn(
            "player tried to join but had incorrect version our "
            "version : {}, their version : {} ",
            HASHED_VERSION, info.hashed_version);
        return;
    }

    // overwrite it so its already there
    int client_id = incoming_client.client_id;

    const auto get_position_for_current_state = [=]() -> vec3 {
        vec3 default_pos = {LOBBY_ORIGIN, 0.f, 0.f};

        // Not in game just spawn them in the lobby
        if (current_menu_state != menu::State::Game) return default_pos;

        switch (current_game_state) {
            case game::InMenu:
            case game::Lobby:
            case game::Paused:
                return default_pos;
            case game::InRound:
            case game::Planning:
                return {0.f, 0.f, 0.f};
            case game::Progression:
                return {PROGRESSION_ORIGIN, 0.f, 0.f};
            case game::Store:
                return {STORE_ORIGIN, 0.f, 0.f};
        }

        return default_pos;
    };

    // create the player if they dont already exist
    if (!players.contains(client_id)) {
        std::shared_ptr<Entity> E = std::make_shared<Entity>();

        // We want to place the character correctly near someone else if
        // possible
        vec3 position = get_position_for_current_state();

        make_player(*E, position);
        players[client_id] = E;
    }

    // update the username
    players[client_id]->get<HasName>().update(info.username);

    // TODO i looked into std::transform but kept getting std::out of range
    // errors
    std::vector<int> ids;
    for (auto& c : server_p->clients) {
        ids.push_back(c.second.client_id);
    }
    // Since we are the host, we can use the internal::Client_t to figure
    // out the id / name
    send_client_packet_to_all(
        ClientPacket{.client_id = SERVER_CLIENT_ID,
                     .msg_type = ClientPacket::MsgType::PlayerJoin,
                     .msg =
                         ClientPacket::PlayerJoinInfo{
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = false,
                         }},
        // ignore the person who sent it to us
        [&](internal::Client_t& client) {
            return client.client_id == incoming_client.client_id;
        });

    send_client_packet_to_all(
        ClientPacket{.client_id = SERVER_CLIENT_ID,
                     .msg_type = ClientPacket::MsgType::PlayerJoin,
                     .msg =
                         ClientPacket::PlayerJoinInfo{
                             .all_clients = ids,
                             // override the client's id with their real one
                             .client_id = incoming_client.client_id,
                             .is_you = true,
                         }},
        // ignore everyone except the one that sent to us
        [&](internal::Client_t& client) {
            return client.client_id != incoming_client.client_id;
        });
}

void Server::process_ping_message(const internal::Client_t& incoming_client,
                                  const ClientPacket& orig_packet) {
    ClientPacket::PingInfo info =
        std::get<ClientPacket::PingInfo>(orig_packet.msg);

    auto pong = now::current_ms();

    ClientPacket packet{
        .channel = Channel::UNRELIABLE_NO_DELAY,
        .client_id = SERVER_CLIENT_ID,
        .msg_type = network::ClientPacket::MsgType::Ping,
        .msg =
            network::ClientPacket::PingInfo{
                .ping = info.ping,
                .pong = pong,
            },
    };
    send_client_packet_to_all(packet, [&](const internal::Client_t& client) {
        return client.client_id != incoming_client.client_id;
    });

    auto player_match = players.find(incoming_client.client_id);
    if (player_match == players.end()) {
        log_warn("We dont have a matching player for {}",
                 incoming_client.client_id);
        return;
    }
    player_match->second->get<HasClientID>().update_ping(pong - info.ping);
}

void Server::process_map_seed_info(const internal::Client_t&,
                                   const ClientPacket& orig_packet) {
    ClientPacket::MapSeedInfo info =
        std::get<ClientPacket::MapSeedInfo>(orig_packet.msg);

    // TODO eventually validate that the update seed call came from the host

    log_info("process map seed info {}", info.seed);
    pharmacy_map->update_seed(info.seed);
}

void Server::server_enqueue_message_string(
    const internal::Client_t& incoming_client, const std::string& msg) {
    incoming_message_queue.push_back(std::make_pair(incoming_client, msg));
}

void Server::server_process_message_string(
    const ClientMessage& client_message) {
    TRACY_ZONE_SCOPED;
    // Note: not using structured binding since they cannot be captured by
    // lambda expr yet
    const internal::Client_t& incoming_client = client_message.first;
    const std::string& msg = client_message.second;

    const ClientPacket packet = network::deserialize_to_packet(msg);
    server_process_packet(incoming_client, packet);
}

void Server::server_process_packet(const internal::Client_t& incoming_client,
                                   const ClientPacket& packet) {
    // log_info("Server: recieved packet {}", packet.msg_type);

    switch (packet.msg_type) {
        case ClientPacket::MsgType::Announcement: {
            return process_announcement_packet(incoming_client, packet);
        } break;
        case ClientPacket::MsgType::PlayerControl: {
            return process_player_control_packet(incoming_client, packet);
        } break;
        case ClientPacket::MsgType::PlayerJoin: {
            return process_player_join_packet(incoming_client, packet);
        } break;
        case ClientPacket::MsgType::PlayerLeave: {
            return process_player_leave_packet(incoming_client, packet);
        } break;
        case ClientPacket::MsgType::Ping: {
            return process_ping_message(incoming_client, packet);
        } break;
        case ClientPacket::MsgType::MapSeed: {
            return process_map_seed_info(incoming_client, packet);
        } break;
        // case ClientPacket::MsgType::PlayerRare: {
        // return process_player_rare_packet(incoming_client, packet);
        // } break;
        default:
            // No clue so lets just send it to everyone except the guy that
            // sent it to us
            send_client_packet_to_all(
                packet, [&](const internal::Client_t& client) {
                    return client.client_id == incoming_client.client_id;
                });
            log_warn("Server: {} not handled yet ", packet.msg_type);
            break;
    }
}

void Server::send_client_packet_to_client(HSteamNetConnection conn,
                                          const ClientPacket& packet) {
    // TODO we should probably see if its worth compressing the data we are
    // sending.

    // TODO write logs for how much data to understand avg packet size per
    // type
    Buffer buffer = serialize_to_buffer(packet);
    server_p->send_message_to_connection(conn, buffer.c_str(),
                                         (uint32) buffer.size());
}

void Server::send_client_packet_to_all(
    const ClientPacket& packet,
    std::function<bool(internal::Client_t&)> exclude) {
    Buffer buffer = serialize_to_buffer(packet);
    server_p->send_message_to_all(buffer.c_str(), (uint32) buffer.size(),
                                  exclude);
}

}  // namespace network
