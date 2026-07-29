#include "ThirdParty/doctest.h"

#include <cstdlib>
#include <string>

#include "RedisSmokeCheck.h"

namespace {

void clear_redis_uri_env_var() { _putenv_s(kRedisUriEnvVar, ""); }

} // namespace

TEST_SUITE("redis_uri") {

// _putenv_s(name, "") deletes the variable rather than setting it to a present-but-empty
// value (there's no way to represent that state via the CRT on Windows), so this also
// covers the "set to empty string" case: both collapse to the same unset environment.
TEST_CASE("returns the default connection uri when KFC_REDIS_URI is unset or empty") {
    clear_redis_uri_env_var();

    CHECK(redis_uri() == kRedisConnectionUri);
}

TEST_CASE("returns the KFC_REDIS_URI value verbatim when it is set to a non-empty string") {
    _putenv_s(kRedisUriEnvVar, "tcp://redis:6379");

    CHECK(redis_uri() == "tcp://redis:6379");

    clear_redis_uri_env_var();
}

}
