#include "doctest/doctest.h"

#include <iostream>

#include "lmdb/lmdb.hpp"

TEST_CASE("dupsort") {
  auto env = lmdb::env{};
  env.open("./DUPSORT.mdb", lmdb::env_open_flags::NOSUBDIR);

  auto txn = lmdb::txn{env};
  auto db = txn.dbi_open(lmdb::dbi_flags::DUPSORT | lmdb::dbi_flags::CREATE);

  SUBCASE("put 2x same key") {
    txn.put(db, "dup_key1", "hello");
    txn.put(db, "dup_key1", "world");
    txn.commit();

    auto read = lmdb::txn{env};
    auto c = lmdb::cursor{read, db};

    auto const e1 = c.get(lmdb::cursor_op::NEXT);
    CHECK(c.count() == 2);
    CHECK(e1->first == "dup_key1");
    CHECK(e1->second == "hello");

    auto const e2 = c.get(lmdb::cursor_op::NEXT);
    CHECK(c.count() == 2);
    CHECK(e2->first == "dup_key1");
    CHECK(e2->second == "world");

    auto const e2_too = c.get(lmdb::cursor_op::GET_CURRENT);
    CHECK(e2_too->first == "dup_key1");
    CHECK(e2_too->second == "world");

    auto const e3 = c.get(lmdb::cursor_op::NEXT);
    CHECK(!e3);

    std::string s;
    for (auto el = c.get(lmdb::cursor_op::FIRST); el;
         el = c.get(lmdb::cursor_op::NEXT)) {
      s += el->second;
    }
    CHECK(s == "helloworld");
  }
}