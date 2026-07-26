#pragma once

// Shared connection string for the docker-compose Postgres instance.
inline constexpr const char* kDatabaseConnectionString = "postgresql://kfc:kfc@localhost:5432/kfc";

// Standalone check that libpqxx is wired into the build; 0 on success.
int run_pqxx_smoke_check();
