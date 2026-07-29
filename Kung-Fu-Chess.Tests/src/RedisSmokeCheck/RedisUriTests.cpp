#include "ThirdParty/doctest.h"

#include <cstdlib>
#include <string>

#include "RedisSmokeCheck.h"

namespace {

// _putenv_s(name, "") deletes the variable rather than setting it to a present-but-empty
// value (there's no way to represent that state via the CRT on Windows); unsetenv() is
// the direct POSIX equivalent of that delete.
void clear_redis_uri_env_var() {
#ifdef _WIN32
    _putenv_s(kRedisUriEnvVar, "");
#else
    unsetenv(kRedisUriEnvVar);
#endif
}

void set_redis_uri_env_var(const std::string& value) {
#ifdef _WIN32
    _putenv_s(kRedisUriEnvVar, value.c_str());
#else
    setenv(kRedisUriEnvVar, value.c_str(), 1);
#endif
}

} // namespace

TEST_SUITE("redis_uri") {

// See clear_redis_uri_env_var()'s comment: unset and empty collapse to the same state.
TEST_CASE("returns the default connection uri when KFC_REDIS_URI is unset or empty") {
    clear_redis_uri_env_var();

    CHECK(redis_uri() == kRedisConnectionUri);
}

TEST_CASE("returns the KFC_REDIS_URI value verbatim when it is set to a non-empty string") {
    set_redis_uri_env_var("tcp://redis:6379");

    CHECK(redis_uri() == "tcp://redis:6379");

    clear_redis_uri_env_var();
}

}
