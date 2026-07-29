#pragma once

#include <string>

// Shared connection string for the docker-compose Postgres instance.
inline constexpr const char* kDatabaseConnectionString = "postgresql://kfc:kfc@localhost:5432/kfc";
// Env var overriding kDatabaseConnectionString.
inline constexpr const char* kDatabaseUriEnvVar = "KFC_DATABASE_URI";

// KFC_DATABASE_URI if set and non-empty, else kDatabaseConnectionString.
std::string database_uri();

// Standalone check that libpqxx is wired into the build; 0 on success.
int run_pqxx_smoke_check();
