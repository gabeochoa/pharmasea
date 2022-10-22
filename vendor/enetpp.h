#include <assert.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "enet.h"

namespace enetpp {

// ugh
class global_state {
   private:
    bool _is_initialized;

   public:
    static global_state& get() {
        static global_state g;
        return g;
    }

   public:
    bool is_initialized() const { return _is_initialized; }

    void initialize() {
        assert(!_is_initialized);
        enet_initialize();
        _is_initialized = true;
    }

    void deinitialize() {
        enet_deinitialize();
        _is_initialized = false;
    }

   private:
    global_state() : _is_initialized(false) {}
};

class client_connect_params {
   public:
    size_t _channel_count;
    enet_uint32 _incoming_bandwidth;
    enet_uint32 _outgoing_bandwidth;
    std::string _server_host_name;
    enet_uint16 _server_port;
    std::chrono::milliseconds _timeout;

   public:
    client_connect_params()
        : _channel_count(0),
          _incoming_bandwidth(0),
          _outgoing_bandwidth(0),
          _server_host_name(),
          _server_port(0),
          _timeout(0) {}

    client_connect_params& set_channel_count(size_t channel_count) {
        _channel_count = channel_count;
        return *this;
    }

    client_connect_params& set_incoming_bandwidth(enet_uint32 bandwidth) {
        _incoming_bandwidth = bandwidth;
        return *this;
    }

    client_connect_params& set_outgoing_bandwidth(enet_uint32 bandwidth) {
        _outgoing_bandwidth = bandwidth;
        return *this;
    }

    client_connect_params& set_server_host_name_and_port(const char* host_name,
                                                         enet_uint16 port) {
        _server_host_name = host_name;
        _server_port = port;
        return *this;
    }

    client_connect_params& set_timeout(std::chrono::milliseconds timeout) {
        _timeout = timeout;
        return *this;
    }

    ENetAddress make_server_address() const {
        ENetAddress address;
        enet_address_set_host(&address, _server_host_name.c_str());
        address.port = _server_port;
        return address;
    }
};
class client_queued_packet {
   public:
    enet_uint8 _channel_id;
    ENetPacket* _packet;

   public:
    client_queued_packet() : _channel_id(0), _packet(nullptr) {}

    client_queued_packet(enet_uint8 channel_id, ENetPacket* packet)
        : _channel_id(channel_id), _packet(packet) {}
};

class client_statistics {
   public:
    std::atomic<int> _round_trip_time_in_ms;
    std::atomic<int> _round_trip_time_variance_in_ms;

   public:
    client_statistics()
        : _round_trip_time_in_ms(0), _round_trip_time_variance_in_ms(0) {}
};
inline void set_current_thread_name(const char* name) {
#ifdef _WIN32

    // https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
    typedef struct tagTHREADNAME_INFO {
        DWORD dwType;      // Must be 0x1000.
        LPCSTR szName;     // Pointer to name (in user addr space).
        DWORD dwThreadID;  // Thread ID (-1=caller thread).
        DWORD dwFlags;     // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = ::GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR),
                       (ULONG_PTR*) &info);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

#else

    // todo - unknown platform

#endif
}

//! IMPORTANT! handler must be thread safe as trace messages come from worker
//! threads as well.
using trace_handler = std::function<void(const std::string&)>;

class client {
   private:
    trace_handler _trace_handler;

    std::queue<client_queued_packet> _packet_queue;
    std::mutex _packet_queue_mutex;

    std::queue<ENetEvent> _event_queue;
    std::queue<ENetEvent>
        _event_queue_copy;  // member variable instead of stack variable to
                            // prevent mallocs?
    std::mutex _event_queue_mutex;

    bool _should_exit_thread;
    std::unique_ptr<std::thread> _thread;

    client_statistics _statistics;

    std::map<std::string, std::function<void()>> on_connected_cbs;
    std::map<std::string, std::function<void()>> on_disconnected_cbs;
    std::map<std::string,
             std::function<void(const enet_uint8* data, size_t data_size)>>
        on_data_received_cbs;

   public:
    client() : _should_exit_thread(false) {}

    ~client() {
        // responsibility of owners to make sure disconnect is always called.
        // not calling disconnect() in destructor due to trace_handler side
        // effects.
        assert(_thread == nullptr);
        assert(_packet_queue.empty());
        assert(_event_queue.empty());
        assert(_event_queue_copy.empty());
    }

    void register_callbacks(
        std::string name, std::function<void()> on_connected,
        std::function<void()> on_disconnected,
        std::function<void(const enet_uint8* data, size_t data_size)>
            on_data_received) {
        on_connected_cbs[name] = on_connected;
        on_disconnected_cbs[name] = on_disconnected;
        on_data_received_cbs[name] = on_data_received;
    }

    void unregister_callbacks(std::string name) {
        on_connected_cbs.erase(name);
        on_disconnected_cbs.erase(name);
        on_data_received_cbs.erase(name);
    }

    void set_trace_handler(trace_handler handler) {
        assert(!is_connecting_or_connected());  // must be set before any
                                                // threads started to be safe
        _trace_handler = handler;
    }

    bool is_connecting_or_connected() const { return _thread != nullptr; }

    void connect(const client_connect_params& params) {
        assert(global_state::get().is_initialized());
        assert(!is_connecting_or_connected());
        assert(params._channel_count > 0);
        assert(params._server_port != 0);
        assert(!params._server_host_name.empty());

        trace("connecting to '" + params._server_host_name + ":" +
              std::to_string(params._server_port) + "'");

        _should_exit_thread = false;
        _thread =
            std::make_unique<std::thread>(&client::run_in_thread, this, params);
    }

    void disconnect() {
        if (_thread != nullptr) {
            _should_exit_thread = true;
            _thread->join();
            _thread.release();
        }

        destroy_all_queued_packets();
        destroy_all_queued_events();
    }

    void send_packet(enet_uint8 channel_id, const enet_uint8* data,
                     size_t data_size, enet_uint32 flags) {
        assert(is_connecting_or_connected());
        if (_thread != nullptr) {
            std::lock_guard<std::mutex> lock(_packet_queue_mutex);
            auto packet = enet_packet_create(data, data_size, flags);
            _packet_queue.emplace(channel_id, packet);
        }
    }

    void consume_events() {
        if (!_event_queue.empty()) {
            //! IMPORTANT! neet to copy the events for consumption to prevent
            //! deadlocks!
            // ex.
            //- event = JoinGameFailed packet received
            //- causes event_handler to call disconnect
            //- disconnect deadlocks as the thread needs a critical
            // section on events to exit
            {
                std::lock_guard<std::mutex> lock(_event_queue_mutex);
                assert(_event_queue_copy.empty());
                _event_queue_copy = _event_queue;
                _event_queue = {};
            }

            bool is_disconnected = false;

            while (!_event_queue_copy.empty()) {
                auto& e = _event_queue_copy.front();
                switch (e.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        for (auto kv : on_connected_cbs) kv.second();
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT: {
                        for (auto kv : on_disconnected_cbs) kv.second();
                        is_disconnected = true;
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE: {
                        for (auto kv : on_data_received_cbs)
                            kv.second(e.packet->data, e.packet->dataLength);
                        enet_packet_destroy(e.packet);
                        break;
                    }

                    case ENET_EVENT_TYPE_NONE:
                    default:
                        assert(false);
                        break;
                }
                _event_queue_copy.pop();
            }

            if (is_disconnected) {
                // cleanup everything internally, make sure the thread is
                // cleaned up.
                disconnect();
            }
        }
    }

    const client_statistics& get_statistics() const { return _statistics; }

   private:
    void destroy_all_queued_packets() {
        std::lock_guard<std::mutex> lock(_packet_queue_mutex);
        while (!_packet_queue.empty()) {
            enet_packet_destroy(_packet_queue.front()._packet);
            _packet_queue.pop();
        }
    }

    void destroy_all_queued_events() {
        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        while (!_event_queue.empty()) {
            destroy_unhandled_event_data(_event_queue.front());
            _event_queue.pop();
        }
    }

    void destroy_unhandled_event_data(ENetEvent& e) {
        if (e.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(e.packet);
        }
    }

    void run_in_thread(const client_connect_params& params) {
        set_current_thread_name("enetpp::client");

        ENetHost* host = enet_host_create(nullptr, 1, params._channel_count,
                                          params._incoming_bandwidth,
                                          params._outgoing_bandwidth);
        if (host == nullptr) {
            trace("enet_host_create failed");
            return;
        }

        auto address = params.make_server_address();
        ENetPeer* peer =
            enet_host_connect(host, &address, params._channel_count, 0);
        if (peer == nullptr) {
            trace("enet_host_connect failed");
            enet_host_destroy(host);
            return;
        }

        enet_uint32 enet_timeout =
            static_cast<enet_uint32>(params._timeout.count());
        enet_peer_timeout(peer, 0, enet_timeout, enet_timeout);

        bool is_disconnecting = false;
        enet_uint32 disconnect_start_time = 0;

        trace("run_in_thread got to loop");

        while (peer != nullptr) {
            _statistics._round_trip_time_in_ms = peer->roundTripTime;
            _statistics._round_trip_time_variance_in_ms =
                peer->roundTripTimeVariance;

            if (_should_exit_thread) {
                if (!is_disconnecting) {
                    enet_peer_disconnect(peer, 0);
                    is_disconnecting = true;
                    disconnect_start_time = enet_time_get();
                } else {
                    if ((enet_time_get() - disconnect_start_time) > 1000) {
                        trace("enet_peer_disconnect took too long");
                        enet_peer_reset(peer);
                        peer = nullptr;
                        break;
                    }
                }
            }

            if (!is_disconnecting) {
                send_queued_packets_in_thread(peer);
            }

            // flush / capture enet events
            // http://lists.cubik.org/pipermail/enet-discuss/2013-September/002240.html
            enet_host_service(host, 0, 0);
            {
                ENetEvent e;
                while (enet_host_check_events(host, &e) > 0) {
                    std::lock_guard<std::mutex> lock(_event_queue_mutex);
                    _event_queue.push(e);
                    if (e.type == ENET_EVENT_TYPE_DISCONNECT) {
                        trace("ENET_EVENT_TYPE_DISCONNECT received");
                        peer = nullptr;
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        enet_host_destroy(host);
    }

    void send_queued_packets_in_thread(ENetPeer* peer) {
        if (!_packet_queue.empty()) {
            std::lock_guard<std::mutex> lock(_packet_queue_mutex);
            while (!_packet_queue.empty()) {
                auto qp = _packet_queue.front();
                _packet_queue.pop();

                if (enet_peer_send(peer, qp._channel_id, qp._packet) != 0) {
                    trace("enet_peer_send failed");
                }

                if (qp._packet->referenceCount == 0) {
                    enet_packet_destroy(qp._packet);
                }
            }
        }
    }

    void trace(const std::string& s) {
        if (_trace_handler != nullptr) {
            _trace_handler(s);
        }
    }
};

template<typename ClientT>
class server_listen_params {
   public:
    using initialize_client_function =
        std::function<void(ClientT& client, const char* ip)>;

   public:
    size_t _max_client_count;
    size_t _channel_count;
    enet_uint32 _incoming_bandwidth;
    enet_uint32 _outgoing_bandwidth;
    enet_uint16 _listen_port;
    std::chrono::milliseconds _peer_timeout;
    initialize_client_function _initialize_client_function;

   public:
    server_listen_params()
        : _max_client_count(0),
          _channel_count(0),
          _incoming_bandwidth(0),
          _outgoing_bandwidth(0),
          _peer_timeout(0) {}

    server_listen_params& set_listen_port(enet_uint16 port) {
        _listen_port = port;
        return *this;
    }

    server_listen_params& set_max_client_count(size_t count) {
        _max_client_count = count;
        return *this;
    }

    server_listen_params& set_channel_count(size_t channel_count) {
        _channel_count = channel_count;
        return *this;
    }

    server_listen_params& set_incoming_bandwidth(enet_uint32 bandwidth) {
        _incoming_bandwidth = bandwidth;
        return *this;
    }

    server_listen_params& set_outgoing_bandwidth(enet_uint32 bandwidth) {
        _outgoing_bandwidth = bandwidth;
        return *this;
    }

    server_listen_params& set_peer_timeout(std::chrono::milliseconds timeout) {
        _peer_timeout = timeout;
        return *this;
    }

    server_listen_params& set_initialize_client_function(
        initialize_client_function f) {
        _initialize_client_function = f;
        return *this;
    }

    ENetAddress make_listen_address() const {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = _listen_port;
        return address;
    }
};

class server_queued_packet {
   public:
    enet_uint8 _channel_id;
    ENetPacket* _packet;
    unsigned int _client_id;

   public:
    server_queued_packet() : _channel_id(0), _packet(nullptr), _client_id(0) {}

    server_queued_packet(enet_uint8 channel_id, ENetPacket* packet,
                         unsigned int client_id)
        : _channel_id(channel_id), _packet(packet), _client_id(client_id) {}
};

// server can't use ENetEvent as ENetPeer is not thread safe. Instead track data
// that is safe.
template<typename ClientT>
class server_event {
   public:
    ENetEventType _event_type;
    enet_uint8 _channel_id;
    ENetPacket* _packet;
    ClientT* _client;

   public:
    server_event()
        : _event_type(ENET_EVENT_TYPE_NONE),
          _channel_id(0),
          _packet(nullptr),
          _client(nullptr) {}

    server_event(ENetEventType event_type, enet_uint8 channel_id,
                 ENetPacket* packet, ClientT* client)
        : _event_type(event_type),
          _channel_id(channel_id),
          _packet(packet),
          _client(client) {}
};

template<typename ClientT>
class server {
   public:
    using event_type = server_event<ClientT>;
    using listen_params_type = server_listen_params<ClientT>;
    using client_ptr_vector = std::vector<ClientT*>;

   private:
    trace_handler _trace_handler;

    bool _should_exit_thread;
    std::unique_ptr<std::thread> _thread;

    // mapping of uid to peer so that sending packets to specific peers is safe.
    std::unordered_map<unsigned int, ENetPeer*> _thread_peer_map;

    client_ptr_vector _connected_clients;

    std::queue<server_queued_packet> _packet_queue;
    std::mutex _packet_queue_mutex;

    std::queue<event_type> _event_queue;
    std::queue<event_type>
        _event_queue_copy;  // member variable instead of stack variable to
                            // prevent mallocs?
    std::mutex _event_queue_mutex;

    std::map<std::string, std::function<void(ClientT&)>> on_connected_cbs;
    std::map<std::string, std::function<void(unsigned int)>>
        on_disconnected_cbs;
    std::map<std::string, std::function<void(ClientT&, const enet_uint8* data,
                                             size_t data_size)>>
        on_data_received_cbs;

   public:
    server() : _should_exit_thread(false) {}

    ~server() {
        // responsibility of owners to make sure stop_listening is always
        // called. not calling stop_listening() in destructor due to
        // trace_handler side effects.
        assert(_thread == nullptr);
        assert(_packet_queue.empty());
        assert(_event_queue.empty());
        assert(_event_queue_copy.empty());
        assert(_connected_clients.empty());
    }

    void register_callbacks(
        std::string name, std::function<void(ClientT&)> on_connected,
        std::function<void(unsigned int)> on_disconnected,
        std::function<void(ClientT&, const enet_uint8* data, size_t data_size)>
            on_data_received) {
        on_connected_cbs[name] = on_connected;
        on_disconnected_cbs[name] = on_disconnected;
        on_data_received_cbs[name] = on_data_received;
    }

    void unregister_callbacks(std::string name) {
        on_connected_cbs.erase(name);
        on_disconnected_cbs.erase(name);
        on_data_received_cbs.erase(name);
    }

    void set_trace_handler(trace_handler handler) {
        assert(!is_listening());  // must be set before any threads started to
                                  // be safe
        _trace_handler = handler;
    }

    bool is_listening() const { return (_thread != nullptr); }

    void start_listening(const listen_params_type& params) {
        assert(global_state::get().is_initialized());
        assert(!is_listening());
        assert(params._max_client_count > 0);
        assert(params._channel_count > 0);
        assert(params._listen_port != 0);
        assert(params._initialize_client_function != nullptr);

        trace("listening on port " + std::to_string(params._listen_port));

        _should_exit_thread = false;
        _thread =
            std::make_unique<std::thread>(&server::run_in_thread, this, params);
    }

    void stop_listening() {
        if (_thread != nullptr) {
            _should_exit_thread = true;
            _thread->join();
            _thread.release();
        }

        destroy_all_queued_packets();
        destroy_all_queued_events();
        delete_all_connected_clients();
    }

    void send_packet_to(unsigned int client_id, enet_uint8 channel_id,
                        const enet_uint8* data, size_t data_size,
                        enet_uint32 flags) {
        assert(is_listening());
        if (_thread != nullptr) {
            std::lock_guard<std::mutex> lock(_packet_queue_mutex);
            auto packet = enet_packet_create(data, data_size, flags);
            _packet_queue.emplace(channel_id, packet, client_id);
        }
    }

    void send_packet_to_all_if(
        enet_uint8 channel_id, const enet_uint8* data, size_t data_size,
        enet_uint32 flags,
        std::function<bool(const ClientT& client)> predicate) {
        assert(is_listening());
        if (_thread != nullptr) {
            std::lock_guard<std::mutex> lock(_packet_queue_mutex);
            auto packet = enet_packet_create(data, data_size, flags);
            for (auto c : _connected_clients) {
                if (predicate(*c)) {
                    _packet_queue.emplace(channel_id, packet, c->get_id());
                }
            }
        }
    }

    void consume_events() {
        if (!_event_queue.empty()) {
            {
                std::lock_guard<std::mutex> lock(_event_queue_mutex);
                assert(_event_queue_copy.empty());
                _event_queue_copy = _event_queue;
                _event_queue = {};
            }

            while (!_event_queue_copy.empty()) {
                auto& e = _event_queue_copy.front();
                trace("EVENT");
                switch (e._event_type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        trace("CONNECT");
                        _connected_clients.push_back(e._client);
                        for (auto kv : on_connected_cbs) kv.second(*e._client);
                        break;
                    }

                    case ENET_EVENT_TYPE_DISCONNECT: {
                        auto iter =
                            std::find(_connected_clients.begin(),
                                      _connected_clients.end(), e._client);
                        assert(iter != _connected_clients.end());
                        _connected_clients.erase(iter);
                        unsigned int client_id = e._client->get_id();
                        delete e._client;
                        for (auto kv : on_disconnected_cbs)
                            kv.second(client_id);
                        break;
                    }

                    case ENET_EVENT_TYPE_RECEIVE: {
                        for (auto kv : on_data_received_cbs)
                            kv.second(*e._client, e._packet->data,
                                      e._packet->dataLength);
                        enet_packet_destroy(e._packet);
                        break;
                    }

                    case ENET_EVENT_TYPE_NONE:
                    default:
                        assert(false);
                        break;
                }
                _event_queue_copy.pop();
            }
        }
    }

    const client_ptr_vector& get_connected_clients() const {
        return _connected_clients;
    }

   private:
    void run_in_thread(const listen_params_type& params) {
        set_current_thread_name("enetpp::server");

        auto address = params.make_listen_address();
        ENetHost* host = enet_host_create(
            &address, params._max_client_count, params._channel_count,
            params._incoming_bandwidth, params._outgoing_bandwidth);
        if (host == nullptr) {
            trace("enet_host_create failed");
        }

        while (host != nullptr) {
            if (_should_exit_thread) {
                disconnect_all_peers_in_thread();
                enet_host_destroy(host);
                host = nullptr;
            }

            if (host != nullptr) {
                send_queued_packets_in_thread();
                capture_events_in_thread(params, host);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void disconnect_all_peers_in_thread() {
        for (auto iter : _thread_peer_map) {
            enet_peer_disconnect_now(iter.second, 0);
            iter.second->data = nullptr;
        }
        _thread_peer_map.clear();
    }

    void send_queued_packets_in_thread() {
        if (!_packet_queue.empty()) {
            std::lock_guard<std::mutex> lock(_packet_queue_mutex);
            while (!_packet_queue.empty()) {
                auto qp = _packet_queue.front();
                _packet_queue.pop();

                auto pi = _thread_peer_map.find(qp._client_id);
                if (pi != _thread_peer_map.end()) {
                    // enet_peer_send fails if state not connected. was getting
                    // random asserts on peers disconnecting and going into
                    // ENET_PEER_STATE_ZOMBIE.
                    if (pi->second->state == ENET_PEER_STATE_CONNECTED) {
                        if (enet_peer_send(pi->second, qp._channel_id,
                                           qp._packet) != 0) {
                            trace("enet_peer_send failed");
                        }

                        if (qp._packet->referenceCount == 0) {
                            enet_packet_destroy(qp._packet);
                        }
                    }
                }
            }
        }
    }

    void capture_events_in_thread(const listen_params_type& params,
                                  ENetHost* host) {
        // http://lists.cubik.org/pipermail/enet-discuss/2013-September/002240.html
        enet_host_service(host, 0, 0);

        ENetEvent e;
        while (enet_host_check_events(host, &e) > 0) {
            switch (e.type) {
                case ENET_EVENT_TYPE_CONNECT: {
                    handle_connect_event_in_thread(params, e);
                    break;
                }

                case ENET_EVENT_TYPE_RECEIVE: {
                    handle_receive_event_in_thread(e);
                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT: {
                    handle_disconnect_event_in_thread(e);
                    break;
                }

                case ENET_EVENT_TYPE_NONE:
                default:
                    assert(false);
                    break;
            }
        }
    }

    void handle_connect_event_in_thread(const listen_params_type& params,
                                        const ENetEvent& e) {
        enet_uint32 enet_timeout =
            static_cast<enet_uint32>(params._peer_timeout.count());
        enet_peer_timeout(e.peer, 0, enet_timeout, enet_timeout);

        char peer_ip[256];
        enet_address_get_host_ip(&e.peer->address, peer_ip, 256);

        //! IMPORTANT! PeerData and it's UID must be created immediately in this
        //! worker thread. Otherwise
        // there is a chance the first few packets are received on the worker
        // thread when the peer is not initialized with data causing them to be
        // discarded.

        auto client = new ClientT();
        params._initialize_client_function(*client, peer_ip);

        assert(e.peer->data == nullptr);
        e.peer->data = client;

        _thread_peer_map[client->get_id()] = e.peer;

        {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            _event_queue.emplace(ENET_EVENT_TYPE_CONNECT, 0, nullptr, client);
        }
    }

    void handle_disconnect_event_in_thread(const ENetEvent& e) {
        auto client = reinterpret_cast<ClientT*>(e.peer->data);
        if (client != nullptr) {
            auto iter = _thread_peer_map.find(client->get_id());
            assert(iter != _thread_peer_map.end());
            assert(iter->second == e.peer);
            e.peer->data = nullptr;
            _thread_peer_map.erase(iter);

            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            _event_queue.emplace(ENET_EVENT_TYPE_DISCONNECT, 0, nullptr,
                                 client);
        }
    }

    void handle_receive_event_in_thread(const ENetEvent& e) {
        auto client = reinterpret_cast<ClientT*>(e.peer->data);
        if (client != nullptr) {
            std::lock_guard<std::mutex> lock(_event_queue_mutex);
            _event_queue.emplace(ENET_EVENT_TYPE_RECEIVE, e.channelID, e.packet,
                                 client);
        }
    }

    void destroy_all_queued_packets() {
        std::lock_guard<std::mutex> lock(_packet_queue_mutex);
        while (!_packet_queue.empty()) {
            enet_packet_destroy(_packet_queue.front()._packet);
            _packet_queue.pop();
        }
    }

    void destroy_all_queued_events() {
        std::lock_guard<std::mutex> lock(_event_queue_mutex);
        while (!_event_queue.empty()) {
            destroy_unhandled_event_data(_event_queue.front());
            _event_queue.pop();
        }
    }

    void delete_all_connected_clients() {
        for (auto c : _connected_clients) {
            delete c;
        }
        _connected_clients.clear();
    }

    void destroy_unhandled_event_data(event_type& e) {
        if (e._event_type == ENET_EVENT_TYPE_CONNECT) {
            delete e._client;
        } else if (e._event_type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(e._packet);
        }
    }

    void trace(const std::string& s) {
        if (_trace_handler != nullptr) {
            _trace_handler(s);
        }
    }
};

}  // namespace enetpp
