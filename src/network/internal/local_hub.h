#pragma once

#include <steam/steamnetworkingtypes.h>

#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace network {
namespace internal {
namespace local {

// Simple in-process hub for "local-only" mode.
//
// Single server, multiple clients (multiple processes are NOT supported).
// Thread-safe via a single mutex; queues are protected by the same mutex.
struct Hub {
    bool server_running = false;

    // Allocate small integer handles; these are NOT real Steam connections.
    HSteamNetConnection next_conn = (HSteamNetConnection) 1;

    std::deque<HSteamNetConnection> disconnect_events;

    // message queues
    std::deque<std::pair<HSteamNetConnection, std::string>> to_server;

    struct Endpoint {
        std::deque<std::string> to_client;
        bool disconnected = false;
    };
    std::unordered_map<HSteamNetConnection, Endpoint> endpoints;

    std::mutex m;
};

// Singleton hub (reset on server startup/shutdown).
Hub& hub();

// Clears all state and disables server_running.
void reset();

// Client-side: request a connection. Returns connection handle.
std::optional<HSteamNetConnection> connect_client();

// Client-side: request disconnect for a connection handle.
void disconnect_client(HSteamNetConnection conn);

// Client -> server
void push_to_server(HSteamNetConnection conn, std::string msg);
std::optional<std::pair<HSteamNetConnection, std::string>> pop_to_server();

// Server -> client
void push_to_client(HSteamNetConnection conn, std::string msg);
std::optional<std::string> pop_to_client(HSteamNetConnection conn);

// Query whether a connection is still alive from the hub's perspective.
bool is_connection_alive(HSteamNetConnection conn);

// Server-side: poll lifecycle events.
std::optional<HSteamNetConnection> pop_disconnect_event();

}  // namespace local
}  // namespace internal
}  // namespace network
