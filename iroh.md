# Evaluating iroh for PharmaSea networking

> Scratch/decision doc. Question: could we switch PharmaSea's multiplayer transport
> from Valve's GameNetworkingSockets (GNS) to [iroh](https://www.iroh.computer/)?
> What would it buy us, what would it cost?

## TL;DR

iroh solves the *one* thing our current stack doesn't: **NAT traversal / connect
without knowing an IP**. Today a client joins by typing the host's public IP
(`conn_info.host_ip_address`, port 770). That only works on LAN or with manual
port-forwarding. iroh would let a player join by a short **node ticket / key**,
with automatic hole-punching and a relay fallback — no port forwarding, works
across the internet.

But iroh is a **Rust** library and PharmaSea is **C++**. That's the whole story:
the win (join-by-key, no port forward) is real and desirable; the cost (FFI
bridge to a Rust lib, or a sidecar process) is real and non-trivial. It is *not*
a drop-in for GNS.

**Recommendation:** don't switch the transport wholesale. Two saner paths:
1. **Keep GNS, steal iroh's idea** — GNS already supports relayed connections &
   signaling (`SteamNetworkingCustomSignaling`); we just aren't using them. If
   the only goal is "join by code instead of IP," we may be able to get there
   without leaving C++.
2. **If we specifically want iroh's P2P mesh / QUIC**, add it behind our existing
   `network::internal::IClient` interface as a third backend (Gns / Local / Iroh)
   via `iroh-ffi`, prototype it, and A/B it. The abstraction already exists —
   see below.

---

## What we have today

Transport: **Valve GameNetworkingSockets** (`vendor/libGameNetworkingSockets.dylib`,
headers in `vendor/steam/`).

Architecture (already nicely layered — this is what makes any swap tractable):

```
network::Info  (RoleInfoMixin: Host / Client role, threads)   src/network/network.h
   ├── network::Server        authoritative game server        src/network/server.*
   └── network::Client        ── wraps ──▶ internal::IClient    src/network/client.*
                                              ├── GnsClient   (GameNetworkingSockets)   src/network/internal/client.*
                                              └── LocalClient (in-process, LOCAL_ONLY)
        internal::Server (GNS listen socket / poll group)       src/network/internal/server.h
```

Key facts that matter for a swap:

- **There is already a transport abstraction**: `internal::IClient` (pure virtual)
  with two impls (`GnsClient`, `LocalClient`). A new `IrohClient` would slot in
  the same way. The server side (`internal::Server`) is *not* abstracted yet —
  it's GNS-specific (`ISteamNetworkingSockets*`, `HSteamListenSocket`,
  `HSteamNetPollGroup`). That would need an interface too.
- **Message API is stringly-typed + channel-tagged**:
  `send_string_to_server(const std::string& msg, Channel channel)`. Payloads are
  serialized with **zpp_bits** (`src/network/serialization.*`), transport just
  moves opaque bytes. Good — transport-agnostic already.
- **Channels map 1:1 onto Steam send flags** (`src/network/internal/channel.h`):
  ```cpp
  enum Channel {
      RELIABLE            = k_nSteamNetworkingSend_Reliable,
      UNRELIABLE          = k_nSteamNetworkingSend_Unreliable,
      UNRELIABLE_NO_DELAY = k_nSteamNetworkingSend_UnreliableNoDelay,
  };
  ```
- **Connection model = raw IP**: `ConnectionInfo.host_ip_address` (default
  `127.0.0.1`), `DEFAULT_PORT = 770` (`src/globals.h`). Client calls
  `set_address(ip)`. No matchmaking, no NAT traversal, no relay.
- **`LOCAL_ONLY`** in-process path exists for dev/single-player.

So the pain point iroh addresses is exactly the weak spot: **you can only play
with someone if you can reach their IP:port directly.**

---

## What iroh is

- Rust library (`iroh` crate, v1.0 shipped Jun 2026). "Dial keys, not IPs."
- You connect to a peer by its **NodeId** (ed25519 public key), not IP. iroh
  finds & maintains the fastest path.
- Built on **QUIC** + **NAT hole-punching** + **relay fallback**. If a direct
  connection can't be punched, traffic is relayed (higher latency, but the
  connection survives). Encrypted + authenticated by default (QUIC/TLS).
- QUIC gives us **both reliable-ordered streams and unreliable datagrams over one
  connection** (RFC 9221) — the same reliable/unreliable split our `Channel` enum
  already expresses.
- Can compile to **WASM/browser** (since 0.33) — relevant only if we ever want a
  web build; browser peers relay rather than punch.

### The mapping is actually clean (conceptually)

| PharmaSea concept        | GNS today                          | iroh equivalent                         |
|--------------------------|------------------------------------|-----------------------------------------|
| Address a peer           | `SteamNetworkingIPAddr` / IP str   | `NodeId` (public key) / node ticket     |
| Reliable send            | `k_nSteamNetworkingSend_Reliable`  | QUIC stream (ordered, reliable)         |
| Unreliable send          | `..._Unreliable[_NoDelay]`         | QUIC datagram (`send_datagram`)         |
| Listen (host)            | `CreateListenSocketIP` + poll grp  | `Endpoint::bind` + `accept()`           |
| Connect (client)         | `ConnectByIPAddress`               | `endpoint.connect(node_id, ALPN)`       |
| NAT traversal            | ❌ (not wired up; needs signaling) | ✅ built in (holepunch + relay)         |
| Join without IP          | ❌                                 | ✅ node ticket / short code             |

Conceptually the transport swap is a small surface. The problem is the language
boundary, not the API shape.

---

## The real costs

### 1. Language boundary (the big one)
iroh is Rust. PharmaSea is C++. Options, worst→best ergonomics:
- **`iroh-ffi`** (official C/FFI bindings) linked as a static/dylib. This is the
  most likely path. We'd wrap it in an `IrohClient : IClient` and an
  `IrohServer`. Cost: add a Rust/cargo build step to the build, ship another
  native lib per platform, hand-manage the FFI boundary (async runtime lives in
  Rust; we'd bridge callbacks the same way GNS callbacks work today).
- **Sidecar process**: run a tiny Rust iroh daemon, talk to it over a local
  socket. Cleaner build isolation, but now there's IPC latency + a second
  process to ship and supervise. Not worth it for a game.
- **Rewrite the net layer in Rust**: no.

None of these are free. Adding a Rust toolchain + `iroh-ffi` to a C++ game's
build (Windows `.vcxproj`, mac dylib, the vendored-lib pattern in `vendor/`) is
the bulk of the work — probably more than the actual netcode changes.

### 2. Server side isn't abstracted
`internal::Server` is GNS-native (poll groups, `HSteamNetConnection`, status
callbacks). We'd have to introduce an `IServer` interface mirroring `IClient`
before an iroh host backend could slot in. Medium refactor, but healthy anyway.

### 3. It's transport only — not netcode
iroh gets bytes between two machines behind NATs. It does **not** give us
prediction, interpolation, reconciliation, snapshot delta-compression, or an
authority model. We already have an authoritative-server design; that all stays
ours. iroh ≠ magic multiplayer.

### 4. P2P architecture baggage (only if we go peer-mesh)
If we used iroh to go true peer-to-peer (no dedicated host), we'd inherit
cheat surface, host-migration, and one-peer-uplink-bottleneck problems. But
PharmaSea is host-authoritative today, so we'd most likely keep
**one host + N clients**, just reachable by key instead of IP. That sidesteps
most P2P baggage — iroh is only replacing "how client finds host."

### 5. Slight QUIC receive-side CPU overhead vs raw UDP
Real but almost certainly irrelevant for a co-op cooking game's packet rate.

---

## Does GNS already do what we actually want?

Probably yes, and this is the honest counterpoint to switching. The concrete
itch is "join without typing an IP / port-forwarding." GNS supports:
- **Relayed connections over Steam Datagram Relay (SDR)** — but that needs a
  Steam appID / Steamworks, which is the real question mark for PharmaSea.
- **`SteamNetworkingCustomSignaling`** (`vendor/steam/steamnetworkingcustomsignaling.h`
  is already vendored!) — you provide your own signaling channel (any out-of-band
  way to exchange a blob: a pastebin code, a tiny rendezvous server, even a
  Discord message) and GNS does P2P + NAT punch **without** Steam. This is
  basically iroh's model, in the lib we already link.

So before adopting a Rust dependency, the cheaper experiment is: **wire up
`SteamNetworkingCustomSignaling` + a dead-simple rendezvous** and get join-by-code
with hole-punching while staying 100% C++.

---

## Suggested path (cheapest → most invasive)

1. **Spike GNS custom signaling** (days, no new deps): prove we can get
   join-by-code + NAT punch with the lib we already have. If it works, we're done
   — iroh's main advantage evaporates.
2. If (1) is painful or GNS's punch is unreliable in practice, **prototype iroh
   behind the existing abstraction**:
   - Add `IServer` interface mirroring `IClient`.
   - Add `IrohClient : IClient` + `IrohServer : IServer` via `iroh-ffi`.
   - Keep `Channel` mapping: RELIABLE→QUIC stream, UNRELIABLE→datagram.
   - Gate behind a build flag / runtime setting; A/B against GNS on real
     cross-NAT connections (measure connect success rate + RTT vs GNS+SDR).
3. Only if the prototype clearly wins on connect-reliability do we take on the
   Rust build-integration cost for real.

## Open questions
- Are we shipping on Steam? If yes, **SDR is basically free NAT traversal** and
  tips the scales hard toward staying on GNS.
- Do we need browser/web builds ever? (iroh WASM is a point in its favor; GNS is
  native-only.)
- What's our tolerance for adding a Rust toolchain to the build/release pipeline?

## References
- iroh: https://www.iroh.computer/ · docs: https://docs.iroh.computer/
- iroh repo: https://github.com/n0-computer/iroh
- iroh-ffi (C bindings): https://github.com/n0-computer/iroh-ffi
- QUIC unreliable datagrams: RFC 9221
- Current code: `src/network/internal/{client,server}.h`, `channel.h`,
  `src/network/network.h`, `src/network/serialization.*`, `src/globals.h`
