
#pragma once

#include <queue>

#include "../src/assert.h"
#include "../src/external_include.h"
#include "enet.h"
// #include "enet.h"

template<typename T>
struct Server {
    struct ServerEvent {
        ENetEventType type;
        int channel_id;
        ENetPacket* packet;
        T* client;
    };

    struct ServerConfig {
        std::function<void(T&, const char*)> init_client;
        int max_clients;
        int channels;
        int in_bandwidth;
        int out_bandwidth;
        int port;
        float timeout = 0;

        ENetAddress address() {
            ENetAddress address;
            address.host = ENET_HOST_ANY;
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
        int client_id = -1;
    };

    std::vector<T*> clients;

    std::queue<QueuedPacket> packets_to_process;
    std::queue<ServerEvent> events_to_process;

    std::map<std::string, std::function<void()>> connected_actions;
    std::map<std::string, std::function<void()>> disconnected_actions;
    std::map<std::string,
             std::function<void(const unsigned char*, std::size_t)>>
        process_data_actions;

    void disconnect() {
        while (!packets_to_process.empty()) {
            enet_packet_destroy(packets_to_process.front().packet);
            packets_to_process.pop();
        }

        while (!events_to_process.empty()) {
            auto& event = events_to_process.front();
            if (event.type == ENET_EVENT_TYPE_CONNECT) {
                delete event.client;
            } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
            events_to_process.pop();
        }

        for (auto c : clients) {
            delete c;
        }
        clients.clear();
    }

    void send_to_client_id(int client_id, int channel_id,
                           const unsigned char* buffer,
                           std::size_t buffer_size) {
        auto packet =
            enet_packet_create(buffer, buffer_size, ENET_PACKET_FLAG_RELIABLE);
        packets_to_process.push({channel_id, packet, client_id});
    }

    void send_to_all(int channel_id, const unsigned char* buffer,
                     std::size_t buffer_size) {
        auto packet =
            enet_packet_create(buffer, buffer_size, ENET_PACKET_FLAG_RELIABLE);
        for (auto client : clients) {
            packets_to_process.push({channel_id, packet, client->id()});
        }
    }

    void process_events() {
        bool disconnected = false;
        while (!events_to_process.empty()) {
            auto& event = events_to_process.front();
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    clients.push_back(event.client);
                    for (auto kv : connected_actions)
                        kv.second(*(event.client));
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT: {
                    auto it =
                        std::find(clients.begin(), clients.end(), event.client);
                    if (it == clients.end()) break;

                    clients.erase(it);
                    int c_id = event.client->id();
                    delete event.client;
                    for (auto kv : disconnected_actions) kv.second(c_id);
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
    }

    void setup_host_and_peer() {
        auto address = connection_config.address();
        ENetHost* host = enet_host_create(
            &address, connection_config.max_clients, connection_config.channels,
            connection_config.in_bandwidth, connection_config.out_bandwidth);

        connection_info.host = host;
    }

    bool tick() {
        M_ASSERT(connection_info.host,
                 "need to call setup_host_and_peer first");

        while (!packets_to_process.empty()) {
            auto packet = packets_to_process.front();
            packets_to_process.pop();
            enet_peer_send(packet.client_id, packet.channel_id, packet.packet);
            enet_packet_destroy(packet.packet);
        }

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

    void register_local_handlers() {
        auto connect = [&](ENetEvent* event) {
            int enet_timeout = this->connection_config.timeout;
            enet_peer_timeout(event->peer, 0, enet_timeout, enet_timeout);

            char peer_ip[256];
            enet_address_get_host_ip(&(event->peer->address), peer_ip, 256);

            auto client = new T();
            connection_config.init_client(*client, peer_ip);

            assert(event->peer->data == nullptr);
            event->peer->data = client;
            // TODO add peer to list
        };

        auto disconnect = [&](ENetEvent* event) {
            auto client = reinterpret_cast<T*>(event->peer->data);
            if (client == nullptr) return;
            event->peer->data = nullptr;
            events_to_process.emplace(
                {ENET_EVENT_TYPE_DISCONNECT, 0, nullptr, client});
        };

        auto recieve = [&](ENetEvent* event) {
            auto client = reinterpret_cast<T*>(event->peer->data);
            if (client == nullptr) return;
            events_to_process.emplace({ENET_EVENT_TYPE_RECEIVE,
                                       event->channelID, event->packet,
                                       client});
        };

        register_connected_action(connect);
        register_disconnected_action(disconnect);
        register_process_action(recieve);
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
