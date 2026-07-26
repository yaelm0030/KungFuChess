#pragma once

#include <stdexcept>
#include <string>

#include "UserRepository.h"

// Always throws, for exercising GameServer's catch-and-degrade policy around ensure_user.
class ThrowingUserRepository : public UserRepository {
public:
    int ensure_user(const std::string& /*username*/) override {
        throw std::runtime_error("ThrowingUserRepository always throws");
    }
};
