# ecs-multiplayer-sim-server

A server-authoritative multiplayer prototype over UDP: a sparse-set ECS simulation ticking at a fixed rate, synced to connected clients with a small custom binary protocol.

## Features

- UDP server, custom binary wire protocol (`Connect` / `Disconnect` / `Spawn` / `Input` / `React` / `Snapshot` / keepalive ping)
- Fixed-timestep simulation loop (accumulator pattern) driving a sparse-set ECS (`SparseSet<T>` — no virtual dispatch, dense contiguous component storage)
- Per-client keepalive: clients must `React` within 60s or get kicked and their entities despawned; server pings idle clients every 20s
- Incoming requests are dispatched to job queues (`Pub`) rather than handled inline on the socket-read thread
- `Swarm.cpp` — a load-testing client that spins up N simulated clients and drives input against the live server

## Architecture

- `MultiplayerServer.cpp` — UDP accept loop, client table (`SparseSet<sockaddr_in>`), protocol dispatch, keepalive thread, sim thread
- `Launcher.cpp` — single interactive client entry point
- `Swarm.cpp` — multi-client stress-test driver
- `localsim.cpp` — standalone benchmark of the ECS tick loop with no networking (20k static + 50k moving entities)
- `library/SparseSet.hpp` — the ECS component storage
- `library/Simulation.hpp` — the fixed-timestep tick loop and entity spawn/kill/input API
- `library/Structs.hpp` — wire-format structs (`Ping`, `Input`, `Position`, `Velocity`) and the `UType` protocol tag
- `library/Client.hpp`/`.cpp` — client-side connection/receive loop
- `library/UIDGenerator.hpp` — entity/client ID allocation

Shared components live in [akinlib](https://github.com/Notero/akinlib), included here as a git submodule.

## Build

```
git clone --recurse-submodules https://github.com/Notero/ecs-multiplayer-sim-server.git
```

(or `git submodule update --init` after a plain clone)

```
cmake -S . -B build
cmake --build build
```

## Run

```
./build/mserver      # starts the UDP server on 127.0.0.1:3300
./build/mclient      # interactive client
./build/swarm <clientCount> <roundCount>   # load test
```

## Results

- ECS component storage (sparse-set, no virtual dispatch) ran ~3x faster than an OOP/virtual-dispatch baseline at 1M entities
- Idle-loop CPU usage cut from ~100% to ~0.2% after moving off a busy-wait tick loop
- `swarm(50, 5)` (50 simulated clients, 5 rounds) teardown time cut from 100+ s to 14.4 s

## Known limitations / next steps

- No client-side interpolation yet — client renders raw server snapshots
- IPv4 localhost only; no NAT traversal or public-internet transport
