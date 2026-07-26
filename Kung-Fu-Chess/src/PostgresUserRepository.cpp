#include "PostgresUserRepository.h"

#include <pqxx/pqxx>

PostgresUserRepository::PostgresUserRepository(const std::string& connection_string)
    : connection_(std::make_unique<pqxx::connection>(connection_string)) {
}

PostgresUserRepository::~PostgresUserRepository() = default;

int PostgresUserRepository::ensure_user(const std::string& username) {
    pqxx::work transaction(*connection_);
    pqxx::result result = transaction.exec(
        "INSERT INTO users (username) VALUES ($1) "
        "ON CONFLICT (username) DO UPDATE SET username = EXCLUDED.username "
        "RETURNING rating",
        pqxx::params{ username });
    transaction.commit();

    return result[0][0].as<int>();
}
