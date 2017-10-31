#include "doctest.h"

#include <iostream>

#include "lmdb/lmdb.hpp"

TEST_CASE("txn") {
  auto env = lmdb::env{};
  env.open("./");

  auto txn = lmdb::txn{env};

  SUBCASE("dbi open works") { txn.dbi_open(); }

  SUBCASE("put and retrieve") {
    auto db = txn.dbi_open();
    txn.put(db, "key1", "hello world");
    auto const v = txn.get(db, "key1");
    CHECK(v == "hello world");
  }
}