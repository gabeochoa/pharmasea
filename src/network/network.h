
#pragma once

#include <cstring>

#include "../../vendor/bitsery/serializer.h"
#include "../external_include.h"
#include "shared.h"
//
#include "../player.h"
#include "../remote_player.h"
#include "client.h"
#include "server.h"

namespace network {

struct Info {
    enum State { s_None, s_Host, s_Client } desired_role = s_None;
    std::shared_ptr<client::Internal> client_info;
    std::shared_ptr<server::Internal> server_info;

    float sinceLastClientSendReset_s = 0.250f;
    float sinceLastClientSend_s = 0.250f;

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

    void update_players(
        float, std::map<int, std::shared_ptr<RemotePlayer>>* remote_players) {
        for (auto kv : this->server_info->clients_to_process) {
            // Check to see if we already have this player?
            if (!remote_players->contains(kv.first)) {
                std::cout << " Adding a new player " << std::endl;
                (*remote_players)[kv.first] = std::make_shared<RemotePlayer>();
                EntityHelper::addEntity((*remote_players)[kv.first]);
            }
            (*remote_players)[kv.first]->update_remotely(
                kv.second.location, kv.second.facing_direction);
        }

        this->server_info->clients_to_process.clear();
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
        client_send_player();
    }

    void client_send_ping(float dt) {
        if (this->client_info->state != client::Internal::State::s_Connected)
            return;

        sinceLastClientSend_s -= dt;
        if (sinceLastClientSend_s >= 0.f) {
            return;
        }
        sinceLastClientSend_s = sinceLastClientSendReset_s;

        ClientPacket ping = ClientPacket({
            .client_id = 1,
        });

        Buffer buffer;
        auto size = bitsery::quickSerialization(OutputAdapter{buffer}, ping);
        network::client::send(this->client_info, buffer.data(), size);
    }

   private:
    void client_send_player() {
        Player me = GLOBALS.get<Player>("player");

        ClientPacket player({
            .client_id = 1,
            .msg_type = ClientPacket::MsgType::PlayerLocation,
            .msg = ClientPacket::PlayerInfo({
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
        auto size = bitsery::quickSerialization(OutputAdapter{buffer}, player);
        network::client::send(this->client_info, buffer.data(), size);
    }
};

}  // namespace network
