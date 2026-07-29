#pragma once

#include <string>

// Shared connection URI for the docker-compose Redis instance.
inline constexpr const char* kRedisConnectionUri = "tcp://127.0.0.1:6379";
// Env var overriding kRedisConnectionUri; project-prefixed (not REDIS_URL) since this
// project's URIs use redis-plus-plus's tcp:// scheme, not the redis:// scheme other
// Redis tooling expects.
inline constexpr const char* kRedisUriEnvVar = "KFC_REDIS_URI";

// KFC_REDIS_URI if set and non-empty, else kRedisConnectionUri.
std::string redis_uri();

// Standalone check that redis-plus-plus is wired into the build; 0 on success.
int run_redis_smoke_check();
