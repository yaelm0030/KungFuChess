#include "ThirdParty/doctest.h"

#include "FakeUserRepository.h"

TEST_SUITE("FakeUserRepository::ensure_user") {

TEST_CASE("a never-seen username is created with the default rating") {
    FakeUserRepository repo;

    CHECK(repo.ensure_user("alice") == 1000);
}

TEST_CASE("calling again for that same username on the same instance returns the identical seeded value, not reset to default") {
    FakeUserRepository repo({ { "carol", 1400 } });

    CHECK(repo.ensure_user("carol") == 1400);
    CHECK(repo.ensure_user("carol") == 1400);
}

TEST_CASE("a username seeded with a non-default rating is returned unchanged, not reset to default") {
    FakeUserRepository repo({ { "alice", 1400 } });

    CHECK(repo.ensure_user("alice") == 1400);
}

TEST_CASE("two distinct usernames on the same instance get independent ratings") {
    FakeUserRepository repo({ { "alice", 1400 } });

    CHECK(repo.ensure_user("bob") == 1000);
    CHECK(repo.ensure_user("alice") == 1400);
}

TEST_CASE("an empty-string username is treated as an ordinary key") {
    FakeUserRepository repo;

    CHECK(repo.ensure_user("") == 1000);
    CHECK(repo.ensure_user("") == 1000);
}

}
