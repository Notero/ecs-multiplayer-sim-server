# 06 — Networked Multiplayer Prototype (Capstone)

Combine the chat server (01), containers (02), thread pool (03), and ECS (05) into a shared physics sandbox with tick-rate state sync between clients.

## Prerequisites

Finish 01, 02, 03, and 05 (04's CMake/sanitizer setup should already be your standard build for everything). This project has no new primitives to learn on its own — it's integration, so treat gaps here as a sign to go back and re-solidify the relevant earlier project rather than powering through.

## Learning objectives

- System integration: composing independently-built pieces into one architecture
- Network state synchronization (tick-based, not per-event)
- Profiling and hot-path optimization under real (not synthetic) load

## Why this project

This is the point of the whole roadmap: one project where every piece is something you built yourself and can explain under questioning. The networking layer is your 01 server (now handling game clients, not chat text). The simulation is your 05 ECS (ticking physics/entity state instead of a benchmark workload). The thread pool from 03 handles per-tick work off the network thread. Your container work from 02 shows up wherever a hand-rolled structure (e.g. a fixed-size input/snapshot ring buffer) beats a generic STL container for this specific access pattern.

## Core concepts to research first

- fixed timestep / tick-rate simulation loop (accumulator pattern)
- state synchronization strategies: full-state snapshot vs delta/event-based sync (pick one deliberately, know why)
- serialization basics for a custom binary protocol (no need for a heavy library — plain struct packing + endianness awareness)
- client-side prediction and server reconciliation (at least conceptually, even if you implement a simplified version)
- interpolation/extrapolation between snapshots for smooth rendering (if you add any visualization)
- profiling your own hot path: where time actually goes once network + sim + threading are combined (not where you assume it goes)

## Build milestones

1. Define the shared simulation state using your 05 ECS (entities = players/objects, components = position/velocity/etc.) and get a local, non-networked physics tick loop running at a fixed rate.
2. Extend the 01 server to accept game clients and route per-client input messages into per-tick jobs submitted to the 03 thread pool.
3. Implement server-authoritative tick: each tick, apply queued inputs to the ECS, step simulation, then broadcast a state snapshot to all clients.
4. Implement client-side receipt and application of snapshots (a minimal client is enough — headless or simple text/console rendering of positions is fine, no need for a full renderer).
5. Introduce a custom ring buffer (from 02, or a purpose-built one) for buffering incoming snapshots/inputs, and justify why it fits this access pattern better than `std::deque`/`std::vector`.
6. Profile the running system under N simulated clients, find the actual hot path, and optimize it — then measure and document the before/after.

## Self-check questions

- Walk through one full tick end-to-end: input arrives → thread pool → ECS update → snapshot → broadcast. Name which project (01/02/03/05) owns each step and why.
- What tradeoff did you make in your sync strategy (full snapshot vs delta), and what would break or need to change if you'd chosen the other one?
- Where was your actual bottleneck when you profiled under load, and was it where you expected before measuring?
- If you had to explain to someone why this project required RAII, templates, threading, and cache-aware design *together* rather than as separate exercises, what would you say?

## Suggested tools/references

- cppreference: [std::chrono](https://en.cppreference.com/w/cpp/chrono) (for tick timing), [std::span](https://en.cppreference.com/w/cpp/container/span) (for serialization buffers)
- Headers: everything from 01/02/03/05, plus `<chrono>`, `<span>`, `<cstdint>` (fixed-width types for wire format)
- `perf` / Instruments for end-to-end profiling under simulated load
- Keep using the 04 CMake + sanitizer build throughout — this is the project most likely to have a race or UB bug hiding in the integration seams
