#pragma once

#include <string>

inline constexpr int kDefaultUserRating = 1000; // matches db/init.sql's DEFAULT 1000

// Maps a username to its persisted rating. ensure_user is an upsert: a
// never-seen username is created with kDefaultUserRating and that value is
// returned; a known username's current rating is returned unchanged.
class UserRepository {
public:
    virtual ~UserRepository() = default;
    virtual int ensure_user(const std::string& username) = 0;
};
