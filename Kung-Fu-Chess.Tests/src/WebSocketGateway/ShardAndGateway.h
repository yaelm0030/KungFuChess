#pragma once

#include "GameEngine.h"
#include "GameShard.h"
#include "UserRepository.h"
#include "WebSocketGateway.h"
#include "../MessageBus/InProcessMessageBus.h"

// Wires a GameShard and a WebSocketGateway together over an in-process bus,
// mirroring how a shard process and a gateway process would be wired over a
// real MessageBus in production.
struct ShardAndGateway {
    InProcessMessageBus bus;
    GameShard shard;
    WebSocketGateway gateway;

    ShardAndGateway(Board board, UserRepository& user_repository,
                     long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell)
        : shard(std::move(board), bus, move_ms_per_cell), gateway(bus, user_repository) {
        shard.start();
    }
    ~ShardAndGateway() {
        gateway.stop();
        shard.stop();
    }
};
