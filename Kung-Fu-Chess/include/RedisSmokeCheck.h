#pragma once

// Shared connection URI for the docker-compose Redis instance.
inline constexpr const char* kRedisConnectionUri = "tcp://127.0.0.1:6379";

// Standalone check that redis-plus-plus is wired into the build; 0 on success.
int run_redis_smoke_check();
