#include "PqxxSmokeCheck.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include <pqxx/pqxx>

std::string database_uri() {
    // std::getenv is flagged C4996 by MSVC in favor of getenv_s; the codebase has no
    // blanket _CRT_SECURE_NO_WARNINGS, so suppress locally at this call site only.
#pragma warning(push)
#pragma warning(disable : 4996)
    const char* env_value = std::getenv(kDatabaseUriEnvVar);
#pragma warning(pop)
    if (env_value != nullptr && env_value[0] != '\0') {
        return env_value;
    }
    return kDatabaseConnectionString;
}

int run_pqxx_smoke_check() {
    try {
        const std::string uri = database_uri();
        pqxx::connection connection(uri);
        pqxx::work transaction(connection);
        pqxx::result result = transaction.exec("SELECT 1");
        transaction.commit();

        if (result.empty() || result[0][0].as<int>() != 1) {
            std::cout << "pqxx smoke check FAILED: unexpected query result\n";
            return 1;
        }

        std::cout << "pqxx smoke check OK: connected to " << uri << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "pqxx smoke check FAILED: " << e.what() << "\n";
        return 1;
    }
}
