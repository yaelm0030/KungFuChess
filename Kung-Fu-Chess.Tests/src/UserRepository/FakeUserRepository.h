#pragma once

#include <map>
#include <string>

#include "UserRepository.h"

class FakeUserRepository : public UserRepository {
public:
    explicit FakeUserRepository(std::map<std::string, int> seed = {}) : ratings_(std::move(seed)) {}

    int ensure_user(const std::string& username) override {
        return ratings_.try_emplace(username, kDefaultUserRating).first->second;
    }

private:
    std::map<std::string, int> ratings_;
};
