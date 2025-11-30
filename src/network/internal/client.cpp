
#include "client.h"

#include <chrono>
#include <thread>
#include <tuple>

#include "../../engine/assert.h"
#include "../../engine/log.h"
#include "../../globals.h"
//
#include "../serialization.h"

namespace network {
namespace internal {

Client::~Client() {
    try {
        send_leave_info_request();
    } catch (std::exception &) {
        // Bitsery can ause this to throw
        //
        //
        // but since we are shutting down anyway,
        // worst case, we can just ignore it
        // and the server will kill the player through heartbeat
    }
}

void Client::set_address(const std::string &ip) {
    address.ParseString(ip.c_str());
    address.m_port = DEFAULT_PORT;
}

void Client::startup() {
    interface = SteamNetworkingSockets();
    if (interface == nullptr) {
        log_warn(
            "Failed to initialize GameNetworkingSockets (SNS) client; cannot "
            "connect");
        running = false;
        return;
    }

    /// [connection int32] Upper limit of buffered pending bytes to be sent,
    /// if this is reached SendMessage will return k_EResultLimitExceeded
    /// Default is 512k (524288 bytes)
    SteamNetworkingUtils()->SetGlobalConfigValueInt32(
        k_ESteamNetworkingConfig_SendBufferSize, 1024 * 1024);

    /// [connection int32] Timeout value (in ms) to use after connection is
    /// established
    SteamNetworkingUtils()->SetGlobalConfigValueInt32(
        k_ESteamNetworkingConfig_TimeoutConnected, 10);
    // NOTE: we might want to make this number larger to handle flakier
    // connections

    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
               (void *) Client::SteamNetConnectionStatusChangedCallback);
    connection = interface->ConnectByIPAddress(address, 1, &opt);
    if (connection == k_HSteamNetConnection_Invalid) {
        log_warn("Failed to create connection");
        return;
    }
    log_info("success connecting");
    running = true;
}
bool Client::poll_incoming_messages() {
    ISteamNetworkingMessage *incoming_msg = nullptr;
    int num_msgs =
        interface->ReceiveMessagesOnConnection(connection, &incoming_msg, 1);
    if (num_msgs == 0) return false;
    if (num_msgs == -1) {
        VALIDATE(false, "Failed checking for messages");
        return false;
    }

    std::string cmd;
    cmd.assign((const char *) incoming_msg->m_pData, incoming_msg->m_cbSize);
    incoming_msg->Release();
    this->process_message_cb(cmd);
    return true;
}

bool Client::run() {
    if (!running) return false;

    auto poll_connection_state_changes = [&]() {
        callback_instance = this;
        interface->RunCallbacks();
    };

    // TODO mess around with this a bit more to see whats a reasonable
    // number
    int i = 0;
    while (poll_incoming_messages()) {
        if (i++ > 10) break;
    }
    poll_connection_state_changes();

    return true;
}

// TODO :DUPE: this is duplicated
// we used to expose this in the header, but that means
// the header has to include shared.h
// .. this way we only include it in the cpp
void send_packet_to_server(Client &client, const ClientPacket &packet) {
    Buffer buffer = serialize_to_buffer(packet);
    client.send_string_to_server(buffer, packet.channel);
}

void Client::send_join_info_request() {
    log_info("client sending join info request");
    ClientPacket packet({.client_id = -1,  // we dont know yet what it is
                         .msg_type = ClientPacket::MsgType::PlayerJoin,
                         .msg = ClientPacket::PlayerJoinInfo({
                             .client_id = -1,  // again
                             .hashed_version = HASHED_VERSION,
                             .is_you = false,
                             .username = username,
                         })});
    send_packet_to_server(*this, packet);
}

void Client::send_leave_info_request() {
    log_info("internal client sending leave info request");
    ClientPacket packet(
        {.client_id = -1,  // TODO we know this, idk where it is tho
         .msg_type = ClientPacket::MsgType::PlayerLeave,
         .msg = ClientPacket::PlayerLeaveInfo({
             .client_id = -1,  // Server will fill this out for us
         })});
    send_packet_to_server(*this, packet);
}

void Client::on_steam_network_connection_status_changed(
    SteamNetConnectionStatusChangedCallback_t *info) {
    log_info("client on stream network connection status changed");
    VALIDATE(info->m_hConn == connection ||
                 connection == k_HSteamNetConnection_Invalid,
             "Connection Status Error");

    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_None:
            // NOTE: We will get callbacks here when we destroy connections.
            // You can ignore these.
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            send_leave_info_request();
            running = false;
            // Print an appropriate message
            if (info->m_eOldState ==
                k_ESteamNetworkingConnectionState_Connecting) {
                // Note: we could distinguish between a timeout, a rejected
                // connection, or some other transport problem.
                log_warn(
                    "We sought the remote host, yet our "
                    "efforts were met with defeat.  ({})",
                    info->m_info.m_szEndDebug);
            } else if (
                info->m_info.m_eState ==
                k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                log_warn(
                    "Alas, troubles beset us; we have lost contact with "
                    "the host.  ({})",
                    info->m_info.m_szEndDebug);
            } else {
                // NOTE: We could check the reason code for a normal
                // disconnection
                log_info("The host hath bidden us farewell.  ({})",
                         info->m_info.m_szEndDebug);
            }

            // Clean up the connection.  This is important!
            // The connection is "closed" in the network sense, but
            // it has not been destroyed.  We must close it on our end, too
            // to finish up.  The reason information do not matter in this
            // case, and we cannot linger because it's already closed on the
            // other end, so we just pass 0's.
            interface->CloseConnection(info->m_hConn, 0, nullptr, false);
            connection = k_HSteamNetConnection_Invalid;
            break;
        }

        case k_ESteamNetworkingConnectionState_Connecting:
            // We will get this callback when we start connecting.
            // We can ignore this.
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            log_info("Connected to server OK");
            send_join_info_request();
            break;

        default:
            // Silences -Wswitch
            break;
    }
}

}  // namespace internal
}  // namespace network
