#pragma once

#include <memory>
#include <string>

#include "UserRepository.h"

namespace pqxx {
class connection;
} // namespace pqxx

// libpqxx-backed UserRepository. Opens one long-lived connection at
// construction and reuses it for every ensure_user call, each wrapped in its
// own short-lived transaction. pqxx types stay out of this header so
// including it doesn't pull libpqxx into every translation unit.
class PostgresUserRepository : public UserRepository {
public:
    explicit PostgresUserRepository(const std::string& connection_string);
    ~PostgresUserRepository() override;

    int ensure_user(const std::string& username) override;

private:
    std::unique_ptr<pqxx::connection> connection_;
};
