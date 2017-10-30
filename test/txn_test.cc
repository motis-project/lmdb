#include "doctest.h"

#include <iostream>

#include "lmdb/lmdb.hpp"

TEST_CASE("txn") {
  auto env = lmdb::env{};
  env.open("./");

  auto txn = lmdb::txn{env};

  SUBCASE("dbi open works") { txn.dbi_open(); }
}