
#include "server.h"

namespace network {

void PharmaSeaAdapter::OnServerClientConnected(int client_id) {
    this->ps_server->client_connected(client_id);
}

void PharmaSeaAdapter::OnServerClientDisconnected(int client_id) {
    this->ps_server->client_disconnected(client_id);
}

}  // namespace network
