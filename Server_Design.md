# Kung-Fu-Chess — Server Design

Companion to `PROJECT_OVERVIEW.md`, which covers the engine internals
(`Board` → `RealTimeArbiter` → `GameEngine` → `Controller`). This document
covers everything *around* the engine: how clients reach a game, how rooms
and matchmaking work, and how the system is expected to scale beyond one
process. The engine itself does not change — every design below treats
`GameEngine` as a black box and preserves one invariant:

> Neither the client nor any gateway decides game rules. `GameEngine` is
> the single authoritative source of truth for game state.

## Where we are today

`GameServer` (`include/GameServer.h`) is a single C++ process that:
- speaks the client protocol directly over a `websocketpp` WebSocket,
- owns exactly one `Controller` / `GameEngine` / `Board` — one game per
  process, no concept of rooms,
- assigns roles by connection order (first two connections = the two
  colors, everyone after = spectators), tracked in in-memory maps guarded
  by `mutex_`,
- looks up/creates users via `UserRepository` (backed by Postgres —
  `db/init.sql` has a single `users` table: `id`, `username`, `rating`).

`docker-compose.yml` currently runs only `db` (Postgres) and `pgadmin`. No
application service is containerized yet.

This is the "basic working server" — transport and game authority are
fused in one class, which is fine for one game but doesn't extend to
rooms, matchmaking, or multiple machines without splitting those
concerns apart first.

## Target architecture

Six components, communicating over an internal message bus rather than
direct calls, so that any piece can be scaled or replaced independently:

1. **API Gateway** — non-realtime HTTP: login, room listing/history.
   Talks to Postgres directly. No game logic.
2. **WebSocket Gateway** — holds live client connections, forwards
   commands to the right Game Shard and pushes state updates back to
   clients. No game logic — purely transport + routing.
3. **Matchmaker** — pairs two "enter without a room" players into a room.
   Owns the waiting queue.
4. **Game Allocator** — decides which Game Shard a given room runs on.
   The answer to "which process owns this room's state," made explicit
   as its own service instead of an implicit routing rule.
5. **Game Server Shards** — run the actual games. Each shard hosts one or
   more `Controller`/`GameEngine` instances (today's `GameServer`, minus
   the WebSocket transport). Authoritative; reached only via the bus.
6. **Observability** — logs, metrics, health checks, load tests. Cuts
   across all of the above.

### Technology choices

| Concern | Choice | Why |
|---|---|---|
| Inter-service bus | Redis Pub/Sub | One piece of infra (Redis) instead of two; NATS is not needed at this scale |
| Ephemeral state (sessions, active rooms, reconnect info, matchmaking queue) | Redis | Same instance as the bus; this data doesn't need durability |
| Durable state (users, games, results, move history) | PostgreSQL | Already in place via `UserRepository`/`db/init.sql` |
| Local multi-service run | Docker Compose | Already the pattern for `db`/`pgadmin` |
| Managed multi-container / scale-out | Kubernetes / K3s | Only matters once there's more than one Game Shard to schedule |

### Rooms semantics

Three ways to enter, sitting in front of the Matchmaker/Allocator:

- **Join existing room** — room number + password → WebSocket Gateway
  asks Allocator which shard owns that room, connects the client there.
  First connection to an empty role slot = black (or white — whichever
  the existing convention ends up being), second = the other color, all
  further connections = spectators.
- **Create room** — client supplies a password, gets back a room number,
  enters as the first player. Allocator assigns a shard to the new room
  at creation time.
- **Enter without a room** — Matchmaker pairs the client with another
  waiting client and creates a room for them, same as "create" from
  there on.

## Why this shape, not more

The six-component picture is the destination, not the first commit.
Building Matchmaker/Allocator/Observability before there's a working
Gateway↔Shard split would mean designing against services that have
nothing to route to yet. Per team direction: smaller-and-working beats
everything-at-once.

## Phased plan

1. **Gateway/Shard split (current focus)** — break `GameServer` into a
   **WebSocket Gateway** (connection handling only) and a **Game Shard**
   (today's `Controller`/`GameEngine`/tick loop, unchanged), talking over
   Redis Pub/Sub. One hardcoded room, no Matchmaker, no Allocator. Goal:
   prove a command survives the Gateway → bus → Shard → bus → Gateway
   round trip and still produces a correct `GameSnapshot`. Compose gains
   `gateway`, `shard`, and `redis` services alongside the existing
   `db`/`pgadmin`.
2. **Rooms** — Game Shard(s) host multiple `Controller` instances keyed
   by room ID; join-by-code/password and create-room flows land; role
   assignment (black/white/spectator) moves from "connection order" to
   "role within a room."
3. **Matchmaker** — waiting-queue service for "enter without a room,"
   creates a room once it pairs two clients.
4. **Game Allocator** — extracted once there's more than one Game Shard
   to choose between; before that, a single shard implicitly "owns"
   every room.
5. **API Gateway** — login/room-listing/history split out from the
   WebSocket path; Postgres access moves here.
6. **Observability** — logs/metrics/health checks/load tests, added once
   there's a distributed system worth observing.

## Explicitly deferred (not in scope until needed)

- Multiple Game Shards / cross-shard load balancing — needs the
  Allocator, which needs rooms to exist first.
- NATS — Redis Pub/Sub covers the bus need at this scale.
- Kubernetes — Docker Compose is sufficient until there's more than one
  shard to orchestrate.
