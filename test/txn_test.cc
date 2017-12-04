#include "doctest.h"

#include "lmdb/lmdb.hpp"

TEST_CASE("txn") {
  auto env = lmdb::env{};
  env.open("./TXN", lmdb::env_open_flags::NOSUBDIR);

  auto txn = lmdb::txn{env};
  auto db = txn.dbi_open();

  SUBCASE("put and get success") {
    txn.put(db, "key1", "hello world");
    CHECK(txn.get(db, "key1") == "hello world");
  }

  SUBCASE("put and get no success (wrong key)") {
    txn.put(db, "key1", "hello world");
    CHECK(!txn.get(db, "key2"));
  }

  SUBCASE("put and get no success (deleted)") {
    txn.put(db, "key1", "hello world");
    txn.del(db, "key1");
    CHECK(!txn.get(db, "key1"));
  }

  SUBCASE("delete no success") {
    txn.put(db, "key2", "hello world");
    CHECK(txn.del(db, "key1") == false);
  }
}