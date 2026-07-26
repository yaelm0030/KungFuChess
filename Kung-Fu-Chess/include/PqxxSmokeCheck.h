#pragma once

// Standalone connectivity check proving libpqxx is wired into the build.
// Connects to the docker-compose Postgres, runs "SELECT 1", prints the
// result to stdout. Returns 0 on success, non-zero on failure.
int run_pqxx_smoke_check();
