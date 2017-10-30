#pragma once

#include "lmdb.h"

#include "lmdb/enum_helper.h"
#include "lmdb/error.h"

#define EX(EXPR)                                         \
  if (auto ec = (EXPR); ec != MDB_SUCCESS) {             \
    throw std::system_error(error::make_error_code(ec)); \
  }

namespace lmdb {

ENUM_FLAGS(env_flags){
    NONE = 0x0,          FIXEDMAP = 0x01,      NOSUBDIR = 0x4000,
    NOSYNC = 0x10000,    RDONLY = 0x20000,     NOMETASYNC = 0x40000,
    WRITEMAP = 0x80000,  MAPASYNC = 0x100000,  NOTLS = 0x200000,
    NOLOCK = 0x400000,   NORDAHEAD = 0x800000, NOMEMINIT = 0x1000000,
    PREVMETA = 0x2000000};

ENUM_FLAGS(dbi_flags){NONE = 0x0,        REVERSEKEY = 0x02, DUPSORT = 0x04,
                      INTEGERKEY = 0x08, DUPFIXED = 0x10,   INTEGERDUP = 0x20,
                      REVERSEDUP = 0x40, CREATE = 0x40000};

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
  env() { EX(mdb_env_create(&env_)); }
  ~env() { mdb_env_close(env_); }

  void open(char const* path, env_flags flags = env_flags::NONE,
            unsigned mode = 0644) {
    EX(mdb_env_open(env_, path, static_cast<unsigned>(flags), mode));
  }

  void set_maxreaders(unsigned int n) { EX(mdb_env_set_maxreaders(env_, n)); }
  void set_mapsize(mdb_size_t size) { EX(mdb_env_set_mapsize(env_, size)); }
  void set_maxdbs(MDB_dbi dbs) { EX(mdb_env_set_maxdbs(env_, dbs)); }

  MDB_env* env_;
};

struct txn final {
  struct dbi final {
    dbi(MDB_env* env, MDB_txn* txn, char const* name, dbi_flags const flags)
        : env_(env) {
      EX(mdb_dbi_open(txn, name, static_cast<unsigned>(flags), &dbi_));
    }
    ~dbi() { mdb_dbi_close(env_, dbi_); }
    MDB_env* env_;
    MDB_dbi dbi_;
  };

  txn(env& env, env_flags const flags = env_flags::NONE) : env_(env.env_) {
    EX(mdb_txn_begin(env.env_, nullptr, static_cast<unsigned>(flags), &txn_));
  }

  txn(env& env, txn& parent, env_flags const flags) : env_(env.env_) {
    EX(mdb_txn_begin(env.env_, parent.txn_, static_cast<unsigned>(flags),
                     &txn_));
  }

  dbi dbi_open(char const* name = nullptr, dbi_flags flags = dbi_flags::NONE) {
    return dbi{env_, txn_, name, flags};
  }

  dbi dbi_open(dbi_flags flags) { return dbi{env_, txn_, nullptr, flags}; }

  MDB_env* env_;
  MDB_txn* txn_;
};

struct cursor final {};

}  // namespace lmdb