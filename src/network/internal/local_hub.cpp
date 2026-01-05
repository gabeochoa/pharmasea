#include "local_hub.h"

namespace network {
namespace internal {
namespace local {

static Hub g_hub;

Hub& hub() { return g_hub; }

void reset() {
    std::lock_guard<std::mutex> lock(g_hub.m);
    g_hub.server_running = false;
    g_hub.next_conn = (HSteamNetConnection) 1;
    g_hub.disconnect_events.clear();
    g_hub.to_server.clear();
    g_hub.endpoints.clear();
}

std::optional<HSteamNetConnection> connect_client() {
    std::lock_guard<std::mutex> lock(g_hub.m);
    if (!g_hub.server_running) return std::nullopt;

    HSteamNetConnection conn = g_hub.next_conn++;
    g_hub.endpoints.emplace(conn, Hub::Endpoint{});
    return conn;
}

void disconnect_client(HSteamNetConnection conn) {
    std::lock_guard<std::mutex> lock(g_hub.m);
    auto it = g_hub.endpoints.find(conn);
    if (it != g_hub.endpoints.end()) {
        it->second.disconnected = true;
    }
    g_hub.disconnect_events.push_back(conn);
}

void push_to_server(HSteamNetConnection conn, std::string msg) {
    std::lock_guard<std::mutex> lock(g_hub.m);
    g_hub.to_server.emplace_back(conn, std::move(msg));
}

std::optional<std::pair<HSteamNetConnection, std::string>> pop_to_server() {
    std::lock_guard<std::mutex> lock(g_hub.m);
    if (g_hub.to_server.empty()) return std::nullopt;
    auto out = std::move(g_hub.to_server.front());
    g_hub.to_server.pop_front();
    return out;
}

void push_to_client(HSteamNetConnection conn, std::string msg) {
    std::lock_guard<std::mutex> lock(g_hub.m);
    auto it = g_hub.endpoints.find(conn);
    if (it == g_hub.endpoints.end()) return;
    it->second.to_client.emplace_back(std::move(msg));
}

std::optional<std::string> pop_to_client(HSteamNetConnection conn) {
    std::lock_guard<std::mutex> lock(g_hub.m);
    auto it = g_hub.endpoints.find(conn);
    if (it == g_hub.endpoints.end()) return std::nullopt;
    if (it->second.to_client.empty()) return std::nullopt;
    std::string out = std::move(it->second.to_client.front());
    it->second.to_client.pop_front();
    return out;
}

bool is_connection_alive(HSteamNetConnection conn) {
    std::lock_guard<std::mutex> lock(g_hub.m);
    if (!g_hub.server_running) return false;
    auto it = g_hub.endpoints.find(conn);
    if (it == g_hub.endpoints.end()) return false;
    if (it->second.disconnected) return false;
    return true;
}

std::optional<HSteamNetConnection> pop_disconnect_event() {
    std::lock_guard<std::mutex> lock(g_hub.m);
    if (g_hub.disconnect_events.empty()) return std::nullopt;
    HSteamNetConnection out = g_hub.disconnect_events.front();
    g_hub.disconnect_events.pop_front();
    return out;
}

}  // namespace local
}  // namespace internal
}  // namespace network

