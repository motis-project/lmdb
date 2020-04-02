#include "doctest/doctest.h"

#include "lmdb/lmdb.hpp"

constexpr auto const TEST_DBI = "two txn";

TEST_CASE("two txn") {
  auto env = lmdb::env{};
  env.set_maxdbs(8);
  env.open("./");

  {
    auto fill_txn = lmdb::txn{env};
    auto dbi = fill_txn.dbi_open(TEST_DBI, lmdb::dbi_flags::CREATE);
    fill_txn.put(dbi, "1", "hello");
    fill_txn.put(dbi, "2", "world");
    fill_txn.commit();
  }

  auto t = lmdb::txn{env};
  auto dbi = t.dbi_open(TEST_DBI);
  t.put(dbi, "3", "bye");
  t.put(dbi, "4", "world");
  CHECK(t.get(dbi, "3") == "bye");
}