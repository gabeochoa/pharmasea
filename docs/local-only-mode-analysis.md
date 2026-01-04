# Local-only mode analysis (`--local`)

## Problem statement

Today the “Host” path starts an in-process server and then connects to it as a client using **GameNetworkingSockets** over IP. This is a good default for LAN/Internet hosting, but it can fail on locked-down corporate machines:

- **Corporate proxy / restricted egress** can break any “phone home” HTTP calls the game makes during networking init (even if the actual multiplayer connection is local).
- **Endpoint security / firewall policy** can block or prompt on servers that listen on “all interfaces” (0.0.0.0), even if the client intends to connect only to localhost.
- In the worst case, machines may block raw UDP sockets or loopback networking entirely; a true in-memory transport would be required (larger change).

The ask is a `--local` mode that supports a “local-only version of the game” to avoid these environments.

## What the game does today (current behavior)

### Host flow (UI)

From `NetworkLayer`, clicking Host calls:

- `network::Info::set_role(network::Info::Role::s_Host)` in `src/network/network.h`

That does:

1. `Server::start(DEFAULT_PORT)` (starts the authoritative server thread)
2. Creates a `network::Client`
3. Immediately calls `client->lock_in_ip()`

`Client::ConnectionInfo::host_ip_address` defaults to `127.0.0.1` (`src/network/client.h`), so host connects to itself.

### Under the hood (transport)

- Server listen socket: `internal::Server::startup()` uses:
  - `SteamNetworkingSockets()`
  - `CreateListenSocketIP(address, ...)`
  - where `address` is created with `address.Clear(); address.m_port = port;` (`src/network/internal/server.h`)
  - **Important**: `address.Clear()` + only setting `m_port` means “bind/listen on all interfaces” (effectively 0.0.0.0:PORT) unless the library treats “unset IP” differently.

- Client connection: `internal::Client::startup()` uses:
  - `ConnectByIPAddress(address, ...)`
  - where `address.ParseString(ip.c_str()); address.m_port = DEFAULT_PORT;` (`src/network/internal/client.cpp`)
  - host uses `ip = 127.0.0.1`

### Proxy-sensitive behavior already present

On init/reset, networking optionally fetches a “remote IP address”:

- `network::Info::reset_connections()` calls `get_remote_ip_address()` when `network::ENABLE_REMOTE_IP` is true (`src/network/network.h`)
- `get_remote_ip_address()` performs an HTTP GET to `http://api.ipify.org/` with no visible timeout handling (`src/engine/network/webrequest.cpp`)

This is a likely failure point on corporate machines with forced proxy / blocked egress (and could potentially hang, depending on the HTTP library’s defaults).

## Why “local server + localhost client” can still fail on corporate machines

Even though the connection target is `127.0.0.1`, there are a few common ways this setup fails in locked-down environments:

1. **Listening on all interfaces triggers policy**
   - Binding to `0.0.0.0:PORT` can be treated as “this app is acting like a server on the network,” triggering endpoint security, admin policy denial, or firewall prompts.
   - Many policies are less strict about binding to **loopback-only** (`127.0.0.1`), because it’s not remotely reachable.

2. **External HTTP call during “network init” blocks/hangs**
   - The `ipify` request is unrelated to local play, but it happens during networking reset and can fail/hang behind proxies.
   - If it blocks on the main thread, the game may appear stuck or broken “when networking is involved”.

3. **Low network timeouts / brittle boot ordering**
   - The code sets `k_ESteamNetworkingConfig_TimeoutConnected = 10` (ms) in both client and server startup. If this value is actually used as a post-connect liveness timeout (per comments), it may be too aggressive in some environments.
   - (This may not be the primary corporate issue, but it’s worth sanity-checking if local connect is flaky.)

4. **Hard restrictions on UDP / socket creation**
   - Some environments block UDP outright or require admin privileges for certain socket behaviors.
   - If this is the actual failure mode, loopback-only listen may still fail; then only an **in-memory** transport truly avoids it.

## What “--local” should mean (recommended definition)

`--local` should guarantee:

- **No external network calls** (no “what is my IP”, no matchmaking, no pings to public services)
- **No listening on external interfaces** (server listens on loopback only)
- **No need to enter/copy IPs** (UI can still show “Local mode” but doesn’t need public IP)

Optionally, `--local` can also:

- Force host mode (or add a UI toggle) if desired for “single-player but server-authoritative” behavior.

## Implementation plan (we should do this): True in-memory transport (no sockets at all)

This replaces the “wire” between host client and server with a local queue/pipe abstraction.

Design sketch:

- Introduce a thin transport interface used by `internal::Client` and `internal::Server`:
  - `send_to_server(bytes, channel)`
  - `poll_from_server()`
  - `poll_from_client()` (server side)
  - connect/disconnect events (or just assume connected)

- Implement:
  - `GnsTransport` (current behavior)
  - `InProcessTransport` (queues; thread-safe; no sockets)

- In `--local` mode:
  - host uses `InProcessTransport`
  - join-by-IP is disabled (because there is no external networking)

Pros:
- Most robust for highly locked-down machines.
- Removes dependence on UDP/socket policies for local play.

Cons:
- Bigger refactor: touches `src/network/internal/client.*`, `src/network/internal/server.*`, and likely call sites.
- Needs careful threading/ownership and backpressure handling.

## Concrete checklist (files to touch)

- **CLI flag / mode switch**
  - `src/game.cpp`: accept `--local` as a known flag; set `network::LOCAL_ONLY = true`
  - `src/engine/globals.h`: add `namespace network { inline bool LOCAL_ONLY = false; }`

- **Transport abstraction + implementations**
  - `src/network/internal/*`: introduce a small “client/server transport” interface
  - Keep existing GNS transport as one implementation
  - Add `InProcessTransport` implementation using in-memory queues (no sockets)
  - Select transport based on `network::LOCAL_ONLY`

- **Local-only UX**
  - In local-only mode, “Join by IP” should be disabled or treated as “Host local” (no remote networking)

## Notes / gotchas

- `ENABLE_DEV_FLAGS` is currently always on (`src/game.h`), and `process_dev_flags()` exits on unknown flags. So `--local` must be added to the allowlist or it will terminate the game.

