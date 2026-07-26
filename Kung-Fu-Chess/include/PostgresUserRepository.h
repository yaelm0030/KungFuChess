#pragma once

#include <memory>
#include <string>

#include "UserRepository.h"

namespace pqxx {
class connection;
} // namespace pqxx

// libpqxx-backed UserRepository. One long-lived connection; pqxx types stay out of this header.
class PostgresUserRepository : public UserRepository {
public:
    explicit PostgresUserRepository(const std::string& connection_string);
    ~PostgresUserRepository() override;

    int ensure_user(const std::string& username) override;

private:
    std::unique_ptr<pqxx::connection> connection_;
};
