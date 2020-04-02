#include "doctest/doctest.h"

#include <iostream>

#include "lmdb/lmdb.hpp"

TEST_CASE("INTEGERKEY") {
  auto env = lmdb::env{};
  env.open("./INTEGERKEY.mdb", lmdb::env_open_flags::NOSUBDIR);

  auto txn = lmdb::txn{env};
  auto db = txn.dbi_open(lmdb::dbi_flags::INTEGERKEY | lmdb::dbi_flags::CREATE);

  SUBCASE("put and get success") {
    txn.put(db, 1, "hello world");
    CHECK(txn.get(db, 1) == "hello world");
  }

  SUBCASE("put and get no success (wrong key)") {
    txn.put(db, 1, "hello world");
    CHECK(!txn.get(db, 2));
  }

  SUBCASE("put and get no success (deleted)") {
    txn.put(db, 1, "hello world");
    txn.del(db, 1);
    CHECK(!txn.get(db, 1));
  }

  SUBCASE("delete no success") {
    txn.put(db, 2, "hello world");
    CHECK(txn.del(db, 1) == false);
  }
}

TEST_CASE("INTEGERDUP") {
  auto env = lmdb::env{};
  env.set_maxdbs(8);
  env.open("./INTEGERDUP.mdb", lmdb::env_open_flags::NOSUBDIR);

  auto txn = lmdb::txn{env};
  auto db = txn.dbi_open(lmdb::dbi_flags::DUPSORT | lmdb::dbi_flags::CREATE);

  SUBCASE("put 2x same key") {
    txn.put(db, 1, "hello");
    txn.put(db, 1, "world");
    txn.commit();

    auto read = lmdb::txn{env};
    auto c = lmdb::cursor{read, db};

    auto const e1 = c.get(lmdb::cursor_op::NEXT);
    CHECK(c.count() == 2);
    CHECK(lmdb::as_int(e1->first) == 1);
    CHECK(e1->second == "hello");

    auto const e2 = c.get(lmdb::cursor_op::NEXT);
    CHECK(c.count() == 2);
    CHECK(lmdb::as_int(e2->first) == 1);
    CHECK(e2->second == "world");

    auto const e2_too = c.get(lmdb::cursor_op::GET_CURRENT);
    CHECK(lmdb::as_int(e2_too->first) == 1);
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