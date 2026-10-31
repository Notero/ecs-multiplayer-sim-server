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

This repo's `CMakeLists.txt` currently defines the `mserver`/`mclient`/`swarm`/`localsim` targets only — a top-level `CMakeLists.txt` wiring this project together with `library` as a subdirectory is not yet included here.

## Run

```
./mserver      # starts the UDP server on 127.0.0.1:3300
./mclient      # interactive client
./swarm <clientCount> <roundCount>   # load test
```
