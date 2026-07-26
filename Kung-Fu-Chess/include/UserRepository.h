#pragma once

#include <string>

inline constexpr int kDefaultUserRating = 1000; // matches db/init.sql's DEFAULT 1000

// ensure_user is an upsert: creates with kDefaultUserRating, or returns the existing rating.
class UserRepository {
public:
    virtual ~UserRepository() = default;
    virtual int ensure_user(const std::string& username) = 0;
};
