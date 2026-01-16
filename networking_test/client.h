

#pragma once

#include <queue>

#include "../src/assert.h"
#include "../src/external_include.h"
// #include "enet.h"

struct Client {
    struct ConnectionConfig {
        int channels;
        int in_bandwidth;
        int out_bandwidth;
        std::string host_name;
        int port;
        float timeout;

        ENetAddress address() {
            ENetAddress address;
            enet_address_set_host(&address, host_name.c_str());
            address.port = this->port;
            return address;
        }
    } connection_config;

    struct ConnectionInfo {
        ENetHost* host = nullptr;
        ENetPeer* peer = nullptr;
    } connection_info;

    struct QueuedPacket {
        int channel_id = -1;
        ENetPacket* packet = nullptr;
    };

    std::queue<QueuedPacket> packets_to_process;
    std::queue<ENetEvent> events_to_process;

    std::map<std::string, std::function<void()>> connected_actions;
    std::map<std::string, std::function<void()>> disconnected_actions;
    std::map<std::string,
             std::function<void(const unsigned char*, std::size_t)>>
        process_data_actions;

    void queue_packet(int channel_id, const unsigned char* buffer,
                      std::size_t buffer_size) {
        auto packet =
            enet_packet_create(buffer, buffer_size, ENET_PACKET_FLAG_RELIABLE);
        packets_to_process.push({channel_id, packet});
    }

    void disconnect() {
        while (!packets_to_process.empty()) {
            enet_packet_destroy(packets_to_process.front().packet);
            packets_to_process.pop();
        }

        while (!events_to_process.empty()) {
            ENetEvent& event = events_to_process.front();
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
            events_to_process.pop();
        }
    }

    void process_events() {
        bool disconnected = false;
        while (!events_to_process.empty()) {
            auto& event = events_to_process.front();
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    for (auto kv : connected_actions) kv.second();
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    for (auto kv : disconnected_actions) kv.second();
                    disconnected = true;
                    break;
                }
                case ENET_EVENT_TYPE_RECEIVE: {
                    for (auto kv : process_data_actions)
                        kv.second(event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                    break;
                }
                default:
                    break;
            }
            events_to_process.pop();
        }
        if (disconnected) {
            disconnect();
        }
    }

    void setup_host_and_peer() {
        M_ASSERT(!connection_config.host_name.empty(), "Please set connection config");

        ENetHost* host = enet_host_create(
            nullptr, 1, connection_config.channels,
            connection_config.in_bandwidth, connection_config.out_bandwidth);
        if (host == nullptr) {
            return;
        }

        auto address = connection_config.address();
        ENetPeer* peer =
            enet_host_connect(host, &address, connection_config.channels, 0);
        if (peer == nullptr) {
            enet_host_destroy(host);
            return;
        }

        enet_peer_timeout(peer, 0, connection_config.timeout,
                          connection_config.timeout);

        //
        connection_info.host = host;
        connection_info.peer = peer;
    }

    bool tick() {
        M_ASSERT(connection_info.host,
                 "Need to call setup_host_and_peer before tick");

        int wait_ms = 1000;

        ENetEvent event;
        while (enet_host_service(connection_info.host, &event, wait_ms)) {
            events_to_process.push(event);
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                return false;
            }
        }
        return true;
    }

    void destroy_host_and_peer() {
        connection_info.peer = nullptr;
        enet_host_destroy(connection_info.host);
    }

    void sent_queued() {
        while (!packets_to_process.empty()) {
            auto packet = packets_to_process.front();
            packets_to_process.pop();
            enet_peer_send(connection_info.peer, packet.channel_id,
                           packet.packet);
            enet_packet_destroy(packet.packet);
        }
    }

    void register_connected_action(std::string name, std::function<void()> cb) {
        connected_actions[name] = cb;
    }

    void unregister_connected_action(std::string name) {
        connected_actions.erase(name);
    }

    void register_disconnected_action(std::string name,
                                      std::function<void()> cb) {
        disconnected_actions[name] = cb;
    }

    void unregister_disconnected_action(std::string name) {
        disconnected_actions.erase(name);
    }

    void register_process_action(
        std::string name,
        std::function<void(const unsigned char*, std::size_t)> cb) {
        process_data_actions[name] = cb;
    }

    void unregister_process_action(std::string name) {
        process_data_actions.erase(name);
    }
};
