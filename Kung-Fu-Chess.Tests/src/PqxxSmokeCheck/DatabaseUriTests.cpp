#include "ThirdParty/doctest.h"

#include <cstdlib>
#include <string>

#include "PqxxSmokeCheck.h"

namespace {

// _putenv_s(name, "") deletes the variable rather than setting it to a present-but-empty
// value (there's no way to represent that state via the CRT on Windows); unsetenv() is
// the direct POSIX equivalent of that delete.
void clear_database_uri_env_var() {
#ifdef _WIN32
    _putenv_s(kDatabaseUriEnvVar, "");
#else
    unsetenv(kDatabaseUriEnvVar);
#endif
}

void set_database_uri_env_var(const std::string& value) {
#ifdef _WIN32
    _putenv_s(kDatabaseUriEnvVar, value.c_str());
#else
    setenv(kDatabaseUriEnvVar, value.c_str(), 1);
#endif
}

} // namespace

TEST_SUITE("database_uri") {

// See clear_database_uri_env_var()'s comment: unset and empty collapse to the same state.
TEST_CASE("returns the default connection string when KFC_DATABASE_URI is unset or empty") {
    clear_database_uri_env_var();

    CHECK(database_uri() == kDatabaseConnectionString);
}

TEST_CASE("returns the KFC_DATABASE_URI value verbatim when it is set to a non-empty string") {
    set_database_uri_env_var("postgresql://kfc:kfc@db:5432/kfc");

    CHECK(database_uri() == "postgresql://kfc:kfc@db:5432/kfc");

    clear_database_uri_env_var();
}

}
