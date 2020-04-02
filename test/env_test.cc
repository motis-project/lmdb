#include "doctest/doctest.h"

#include "lmdb/lmdb.hpp"

TEST_CASE("env") {
  auto env = lmdb::env{};

  SUBCASE("zero readers throws") {
    CHECK_THROWS_AS(env.set_maxreaders(0), std::system_error);
  }

  SUBCASE("open with non-existent directory throws") {
    CHECK_THROWS_AS(
        env.open("/does_not_exist_1337",
                 lmdb::env_open_flags::FIXEDMAP | lmdb::env_open_flags::NOSYNC,
                 0600),
        std::system_error);
  }
}