

#include "client.h"
#include "server.h"

void setup_client(Client& client) {
    client.connection_config = Client::ConnectionConfig({
        .channels = 1,
        .in_bandwidth = 0,
        .out_bandwidth = 0,
        .host_name = "localhost",
        .port = 777,
        .timeout = 0,
    });
    client.setup_host_and_peer();

    auto on_connected = [&]() { std::cout << "connected" << std::endl; };
    auto on_disconnected = [&]() { std::cout << "disconnected" << std::endl; };
    auto on_data_received = [&](const enet_uint8* data, size_t data_size) {
        std::cout << "data recieved" << std::endl;
    };

    client.register_connected_action("client", on_connected);
    client.register_disconnected_action("client", on_disconnected);
    client.register_process_action("client", on_data_received);
}

int next_uid = 0;
struct ThinClient {
    int uid;
    int id() { return uid; }
};

void setup_server(Server<ThinClient>& server) {
    server.connection_config = Server<ThinClient>::ServerConfig{
        .init_client =
            [&](ThinClient& client, const char*) {
                client.uid = next_uid;
                next_uid++;
            },
        .max_clients = 3,
        .channels = 1,
        .in_bandwidth = 0,
        .out_bandwidth = 0,
        .port = 777,
        .timeout = 0,
    };
    server.setup_host_and_peer();

    auto on_connected = [&]() { std::cout << "connected" << std::endl; };
    auto on_disconnected = [&]() { std::cout << "disconnected" << std::endl; };
    auto on_data_received = [&](const enet_uint8* data, size_t data_size) {
        std::cout << "data recieved" << std::endl;
    };

    server.register_connected_action("server", on_connected);
    server.register_disconnected_action("server", on_disconnected);
    server.register_process_action("server", on_data_received);
}

int main() {
    enet_initialize();

    Server<ThinClient> server;
    setup_server(server);

    Client client;
    setup_client(client);

    int i = 0;
    while (true) {
        // Server
        {
            std::vector<unsigned char> hi;
            hi.push_back(1);
            server.send_to_all(0, hi.data(), 1);
        }
        // Client
        {
            client.tick();
            // send stuff
            std::vector<unsigned char> hi;
            hi.push_back(1);
            client.queue_packet(0, hi.data(), 1);
            client.process_events();
        }
        i++;
        if (i > 10) {
            break;
        }
    }

    client.disconnect();
    server.disconnect();
    enet_deinitialize();
}
