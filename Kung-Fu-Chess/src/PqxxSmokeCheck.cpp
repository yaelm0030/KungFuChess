#include "PqxxSmokeCheck.h"

#include <iostream>

#include <pqxx/pqxx>

namespace {

constexpr const char* kConnectionString = "postgresql://kfc:kfc@localhost:5432/kfc";

} // namespace

int run_pqxx_smoke_check() {
    try {
        pqxx::connection connection(kConnectionString);
        pqxx::work transaction(connection);
        pqxx::result result = transaction.exec("SELECT 1");
        transaction.commit();

        if (result.empty() || result[0][0].as<int>() != 1) {
            std::cout << "pqxx smoke check FAILED: unexpected query result\n";
            return 1;
        }

        std::cout << "pqxx smoke check OK: connected to " << kConnectionString << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "pqxx smoke check FAILED: " << e.what() << "\n";
        return 1;
    }
}
