#include "doctest.h"

#include <iostream>

#include "lmdb/lmdb.hpp"

TEST_CASE("iteration") {
  auto env = lmdb::env{};
  env.open("./ITERATION.mdb", lmdb::env_open_flags::NOSUBDIR);

  auto txn = lmdb::txn{env};
  auto db = txn.dbi_open(lmdb::dbi_flags::DUPSORT);

  txn.put(db, "key0", "val0");
  txn.put(db, "key2", "val2");
  txn.put(db, "key3", "val3a");
  txn.put(db, "key3", "val3b");
  txn.put(db, "key7", "val7");
  txn.put(db, "key5", "val5");

  auto c = lmdb::cursor{txn, db};

  std::string keys;
  std::string values;
  for (auto el = c.get(lmdb::cursor_op::SET_RANGE, "key1"); el->first < "key6";
       el = c.get(lmdb::cursor_op::NEXT)) {
    keys += el->first;
    values += el->second;
  }

  CHECK(keys == "key2key3key3key5");
  CHECK(values == "val2val3aval3bval5");
}
