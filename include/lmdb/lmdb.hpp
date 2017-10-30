#pragma once

#include "lmdb/enum_helper.hpp"
#include "lmdb/error.hpp"
#include "lmdb/lmdb.h"

#define CHECKED(EXPR)                                      \
  {                                                        \
    if (auto ec = (EXPR); ec != MDB_SUCCESS) {             \
      throw std::system_error(error::make_error_code(ec)); \
    }                                                      \
  }

namespace lmdb {

ENUM_FLAGS(env_flags){
    FIXEDMAP = 0x01,      NOSUBDIR = 0x4000,     NOSYNC = 0x10000,
    RDONLY = 0x20000,     NOMETASYNC = 0x40000,  WRITEMAP = 0x80000,
    MAPASYNC = 0x100000,  NOTLS = 0x200000,      NOLOCK = 0x400000,
    NORDAHEAD = 0x800000, NOMEMINIT = 0x1000000, PREVMETA = 0x2000000};

ENUM_FLAGS(dbi_flags){REVERSEKEY = 0x02, DUPSORT = 0x04,    INTEGERKEY = 0x08,
                      DUPFIXED = 0x10,   INTEGERDUP = 0x20, REVERSEDUP = 0x40,
                      CREATE = 0x40000};

ENUM_FLAGS(put_flags){NOOVERWRITE = 0x10, NODUPDATA = 0x20, CURRENT = 0x40,
                      RESERVE = 0x10000,  APPEND = 0x20000, APPENDDUP = 0x40000,
                      MULTIPLE = 0x80000};

enum class cursor_op {
  FIRST,
  FIRST_DUP,
  GET_BOTH,
  GET_BOTH_RANGE,
  GET_CURRENT,
  GET_MULTIPLE,
  LAST,
  LAST_DUP,
  NEXT,
  NEXT_DUP,
  NEXT_MULTIPLE,
  NEXT_NODUP,
  PREV,
  PREV_DUP,
  PREV_NODUP,
  SET,
  SET_KEY,
  SET_RANGE,
  PREV_MULTIPLE
};

struct env final {
  env() { CHECKED(mdb_env_create(&env_)); }

  env(char const* path, env_flags flags, int mode) : env() {
    open(path, flags, mode);
  }

  ~env() { mdb_env_close(env_); }

  void set_maxreaders(unsigned int readers) {
    CHECKED(mdb_env_set_maxreaders(env_, readers));
  }

  void set_mapsize(mdb_size_t size) {
    CHECKED(mdb_env_set_mapsize(env_, size));
  }

  void set_maxdbs(MDB_dbi dbs) { CHECKED(mdb_env_set_maxdbs(env_, dbs)); }

  void open(char const* path, env_flags flags, int mode) {
    CHECKED(mdb_env_open(env_, path,
                         static_cast<std::underlying_type_t<env_flags>>(flags),
                         mode));
  }

  MDB_env* env_;
};

struct txn {};

struct cursor {};

}  // namespace lmdb