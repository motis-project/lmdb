#pragma once

#include <cassert>
#include <cstring>
#include <limits>
#include <optional>
#include <string_view>

#include "lmdb.h"

#include "lmdb/enum_helper.h"
#include "lmdb/error.h"

#define EX(EXPR)                                         \
  if (auto const ec = (EXPR); ec != MDB_SUCCESS) {       \
    throw std::system_error{error::make_error_code(ec)}; \
  }

namespace lmdb {

ENUM_FLAGS(env_open_flags){
    NONE = 0x0,

    // pointers to data items will be constant; highly experimental!
    FIXEDMAP = 0x01,

    // path is the database file itself (+ {}-lock), not a directory
    NOSUBDIR = 0x4000,

    // no flush; crash may undo the last commited transactions (ACI)
    // if filesystem preserves write order and WRITEMAP is not used
    // note: use (MAPASYNC | WRITEMAP) for WRITEMAP
    NOSYNC = 0x10000,

    // no writes allowed; lock file will be modified except on RO filesystems
    RDONLY = 0x20000,

    // flush only once per transaction; omit metadata flush
    // flush on none-RDONLY commit or env::sync()
    // crash may undo the last commited transaction (ACID without D)
    NOMETASYNC = 0x40000,

    // writeable memory map unless RDONLY is set
    // faster for DBs that fit in RAM
    // incompatible with nested transactions
    WRITEMAP = 0x80000,

    // use asynchronous flushes to disk
    // a system crash can then corrupt the database
    // env::sync() for on-disk database integrity until next commit
    MAPASYNC = 0x100000,

    // no thread-local storage
    // txn::reset() keeps the slot reserved for the txn object
    // allows for
    // - more than one RO transaction per thread
    // - one RO transaction for multiple threads
    // user threads per OS thread -> user serializes writes
    NOTLS = 0x200000,

    // user manages concurrency (single-writer)
    NOLOCK = 0x400000,

    // random read performance for DB > RAM and RAM is full
    NORDAHEAD = 0x800000,

    // don't zero out memory
    // avoids persisting leftover data from other code (security)
    NOMEMINIT = 0x1000000,

    // open the env with the previous meta page.
    // loses the latest transaction, but may help with curruption
    PREVMETA = 0x2000000};

ENUM_FLAGS(txn_flags){NONE = 0x0,

                      // transaction will not perform any write operations
                      RDONLY = 0x20000,

                      // no flush when commiting this transaction
                      NOSYNC = 0x10000,

                      // flush system buffer, but omit metadata flush
                      NOMETASYNC = 0x40000};

ENUM_FLAGS(dbi_flags){
    NONE = 0x0,

    // sort in reverse order
    REVERSEKEY = 0x02,

    // keys may have multiple data items, stored in sorted order
    DUPSORT = 0x04,

    // keys are binary integers in native byte order: unsigned int or mdb_size_t
    INTEGERKEY = 0x08,

    // only in combination with DUPSORT: data items are same size
    // enables GET_MULTIPLE,  NEXT_MULTIPLE, and PREV_MULTIPLE cursor ops
    DUPFIXED = 0x10,

    // duplicate data items are binary integers similar to INTEGERKEY keys
    INTEGERDUP = 0x20,

    // duplicate data items should be compared as strings in reverse order
    REVERSEDUP = 0x40,

    // create named database if non-existent (not allowed in RO txn / RO env)
    CREATE = 0x40000};

ENUM_FLAGS(put_flags){
    NONE = 0x0,

    // enter the new key/data pair only if not already contained
    // returns KEYEXIST if key/value pair is already contained even with DUPSORT
    NOOVERWRITE = 0x10,

    // enter key/value pair only if not already contained
    // only with DUPSORT database
    // returns KEYEXIST if key/value pair is already contained
    NODUPDATA = 0x20,

    // replace current item; key parameter still required (has to match)
    // ideal: size of new is same as old size; otherwise delete+insert
    CURRENT = 0x40,

    // only make place for data of the given size; no copy
    // returns a pointer to the reserved space to be filled later
    // returned pointer valid until next update operation / transaction end
    // not compatible with DUPSORT!
    RESERVE = 0x10000,

    // insert to end of database without key comparison (useful for bulk insert)
    // returns MDB_KEYEXIST if not sorted
    APPEND = 0x20000,

    // as APPEND, but for sorted dup data
    APPENDDUP = 0x40000,

    // only with DUPFIXED: store multiple contiguous data elements
    // data argument must be an array of two MDB_vals:
    // param=[
    //  {mv_size="size of a singe data element", mv_data="ptr to begin of arr"},
    //  {mv_size="number of data elements to store", mv_data=unused}
    // ]
    // after call: param[1].mv_size = number of elements written
    MULTIPLE = 0x80000};

enum class cursor_op {
  FIRST,  // go to first entry
  FIRST_DUP,  // go to first entry of current key (DUPSORT)
  GET_BOTH,  // go to entry (DUPSORT)
  GET_BOTH_RANGE,  // go to entry with nearest data (DUPSORT)
  GET_CURRENT,  // return current entry
  GET_MULTIPLE,  // return key and up to a page of duplicate values (DUPFIXED)
  LAST,  // go to last entry
  LAST_DUP,  // go to last entry of current key (DUPSORT)
  NEXT,  // go to next entry
  NEXT_DUP,  // go to next entry of current key (DUPSORT)
  NEXT_MULTIPLE,  // return key and up to a page of duplicate values of next
  // entry (DUPFIXED)
  NEXT_NODUP,  // go to first entry of next key
  PREV,  // go to previous entry
  PREV_DUP,  // go to previous entry of current key (DUPSORT)
  PREV_NODUP,  // go to last entry of previous key
  SET,  // go to specified key
  SET_KEY,  // go to specified key and return key + value
  SET_RANGE,  // go to first key greater or equal specified key
  PREV_MULTIPLE  // go to previous page and return key and up to a page of
  // duplicate entries (DUPFIXED)
};

struct env final {
  env() : env_{nullptr} { EX(mdb_env_create(&env_)); }

  ~env() {
    mdb_env_close(env_);
    env_ = nullptr;
  }

  env(env&& e) noexcept : env_{e.env_} { e.env_ = nullptr; }

  env& operator=(env&& e) noexcept {
    env_ = e.env_;
    e.env_ = nullptr;
    return *this;
  }

  env(env const&) = delete;
  env& operator=(env const&) = delete;

  void open(char const* path, env_open_flags flags = env_open_flags::NONE,
            mdb_mode_t mode = 0644) {
    EX(mdb_env_open(env_, path, static_cast<unsigned>(flags), mode));
    is_open_ = true;
  }

  void set_maxreaders(unsigned int n) { EX(mdb_env_set_maxreaders(env_, n)); }
  void set_mapsize(mdb_size_t size) { EX(mdb_env_set_mapsize(env_, size)); }
  void set_maxdbs(MDB_dbi dbs) { EX(mdb_env_set_maxdbs(env_, dbs)); }

  MDB_stat stat() {
    MDB_stat stat;
    EX(mdb_env_stat(env_, &stat));
    return stat;
  }

  void sync() { EX(mdb_env_sync(env_, 0)); }
  void force_sync() { EX(mdb_env_sync(env_, 1)); }

  bool is_open() const { return is_open_; }

  MDB_env* env_;
  bool is_open_ = false;
};

template <typename T>
inline std::enable_if_t<std::is_integral_v<T>, MDB_val> to_mdb_val(T const& s) {
  static_assert(sizeof(s) == sizeof(size_t) || sizeof(s) == sizeof(unsigned));
  return MDB_val{sizeof(T), const_cast<void*>(  // NOLINT
                                reinterpret_cast<void const*>(&s))};
}

template <int N>
inline MDB_val to_mdb_val(char const (&s)[N]) {
  return MDB_val{
      N, const_cast<void*>(reinterpret_cast<void const*>(s))};  // NOLINT
}

inline MDB_val to_mdb_val(std::string_view s) {
  return MDB_val{s.size(), const_cast<char*>(s.data())};  // NOLINT
}

inline std::string_view from_mdb_val(MDB_val v) {
  return {static_cast<char const*>(v.mv_data), v.mv_size};
}

template <typename T = int>
inline T as_int(std::string_view s) {
  assert(s.length() >= sizeof(T));
  T t;
  std::memcpy(&t, s.data(), sizeof(t));
  return t;
}

struct txn final {
  struct dbi final {
    dbi(MDB_txn* txn, char const* name, dbi_flags const flags)
        : txn_{txn}, dbi_{std::numeric_limits<MDB_dbi>::max()} {
      EX(mdb_dbi_open(txn, name, static_cast<unsigned>(flags), &dbi_));
    }

    dbi(MDB_txn* txn, MDB_dbi dbi) : txn_{txn}, dbi_{dbi} {}

    MDB_stat stat() {
      MDB_stat stat;
      EX(mdb_stat(txn_, dbi_, &stat));
      return stat;
    }

    void close() { mdb_dbi_close(mdb_txn_env(txn_), dbi_); }
    void clear() { EX(mdb_drop(txn_, dbi_, 0)); }
    void remove() { mdb_drop(txn_, dbi_, 1); }

    MDB_txn* txn_;
    MDB_dbi dbi_;
  };

  explicit txn(env& env, txn_flags const flags = txn_flags::NONE)
      : committed_{false}, txn_{nullptr} {
    EX(mdb_txn_begin(env.env_, nullptr, static_cast<unsigned>(flags), &txn_));
  }

  txn(env& env, txn& parent, txn_flags const flags)
      : committed_{false}, txn_{nullptr} {
    EX(mdb_txn_begin(env.env_, parent.txn_, static_cast<unsigned>(flags),
                     &txn_));
  }

  txn(txn&& t) noexcept : committed_{t.committed_}, txn_{t.txn_} {
    t.txn_ = nullptr;
  }

  txn& operator=(txn&& t) noexcept {
    committed_ = t.committed_;
    txn_ = t.txn_;
    t.txn_ = nullptr;
    return *this;
  }

  txn(txn const&) = delete;
  txn& operator=(txn const&) = delete;

  ~txn() {
    if (!committed_) {
      mdb_txn_abort(txn_);
    }
  }

  void commit() {
    committed_ = true;
    mdb_txn_commit(txn_);
  }

  void clear() {
    mdb_txn_reset(txn_);
    EX(mdb_txn_renew(txn_));
  }

  dbi dbi_open(char const* name = nullptr, dbi_flags flags = dbi_flags::NONE) {
    return {txn_, name, flags};
  }

  dbi dbi_open(dbi_flags flags) { return {txn_, nullptr, flags}; }

  template <typename T>
  void put(dbi& dbi, T key, std::string_view value,
           put_flags const flags = put_flags::NONE) {
    auto k = to_mdb_val(key);
    auto v = to_mdb_val(value);
    EX(mdb_put(txn_, dbi.dbi_, &k, &v, static_cast<unsigned>(flags)));
  }

  template <typename T>
  bool put_nodupdata(dbi& dbi, T key, std::string_view value,
                     put_flags const flags = put_flags::NONE) {
    auto k = to_mdb_val(key);
    auto v = to_mdb_val(value);
    if (auto const ec =
            mdb_put(txn_, dbi.dbi_, &k, &v,
                    static_cast<unsigned>(flags | put_flags::NODUPDATA));
        ec != MDB_KEYEXIST && ec != MDB_SUCCESS) {
      throw std::system_error{error::make_error_code(ec)};
    } else {
      return ec != MDB_KEYEXIST;
    }
  }

  template <typename T>
  std::optional<std::string_view> get(dbi& dbi, T key) {
    auto k = to_mdb_val(key);
    auto v = MDB_val{0, nullptr};
    auto const ec = mdb_get(txn_, dbi.dbi_, &k, &v);
    switch (ec) {
      case MDB_SUCCESS: return from_mdb_val(v);
      case MDB_NOTFOUND: return {};
      default: throw std::system_error{error::make_error_code(ec)};
    }
  }

  template <typename T>
  bool del(dbi& dbi, T key) {
    auto k = to_mdb_val(key);
    auto const ec = mdb_del(txn_, dbi.dbi_, &k, nullptr);
    switch (ec) {
      case MDB_SUCCESS: return true;
      case MDB_NOTFOUND: return false;
      default: throw std::system_error{error::make_error_code(ec)};
    }
  }

  template <typename T>
  bool del_dupdata(dbi& dbi, T key, std::string_view value) {
    auto k = to_mdb_val(key);
    auto v = to_mdb_val(value);
    switch (auto const ec = mdb_del(txn_, dbi.dbi_, &k, &v); ec) {
      case MDB_SUCCESS: return true;
      case MDB_NOTFOUND: return false;
      default: throw std::system_error{error::make_error_code(ec)};
    }
  }

  bool committed_;
  MDB_txn* txn_;
};

struct cursor final {
  using opt_entry =
      std::optional<std::pair<std::string_view, std::string_view>>;

  template <typename T>
  using opt_int_entry = std::optional<std::pair<T, std::string_view>>;

  cursor(txn& txn, txn::dbi& dbi) : cursor_{nullptr} {
    EX(mdb_cursor_open(txn.txn_, dbi.dbi_, &cursor_));
  }

  cursor(cursor&& c) noexcept : cursor_{c.cursor_} { c.cursor_ = nullptr; }

  cursor& operator=(cursor&& c) noexcept {
    cursor_ = c.cursor_;
    c.cursor_ = nullptr;
    return *this;
  }

  cursor(cursor const&) = delete;
  cursor& operator=(cursor const&) = delete;

  ~cursor() {
    if (cursor_ != nullptr) {
      mdb_cursor_close(cursor_);
      cursor_ = nullptr;
    }
  }

  void reset() { cursor_ = nullptr; }

  void commit() { cursor_ = nullptr; }

  void renew(txn& t) { EX(mdb_cursor_renew(t.txn_, cursor_)); }

  template <int N>
  opt_entry get(cursor_op const op, char const (&s)[N]) {
    auto k = to_mdb_val<N>(s);
    return get(op, &k);
  }

  opt_entry get(cursor_op const op, std::string const& key) {
    return get(op, key.operator std::string_view());
  }

  opt_entry get(cursor_op const op, std::string_view key) {
    auto k = to_mdb_val(key);
    return get(op, &k);
  }

  template <typename T>
  std::enable_if_t<std::is_integral_v<T>, opt_int_entry<T>> get(
      cursor_op const op) {
    auto k = MDB_val{0, nullptr};
    auto r = get(op, &k);
    return r ? std::make_optional(
                   std::make_pair(as_int<std::decay_t<T>>(r->first), r->second))
             : std::nullopt;
  }

  opt_entry get(cursor_op const op) {
    auto k = MDB_val{};
    return get(op, &k);
  }

  template <typename T>
  opt_int_entry<T> get(cursor_op op, T const& key) {
    auto k = to_mdb_val(key);
    auto r = get(op, &k);
    return r ? std::make_optional(
                   std::make_pair(as_int<T>(r->first), r->second))
             : std::nullopt;
  }

  opt_entry get(cursor_op const op, MDB_val* k) {
    auto v = MDB_val{};
    switch (auto const ec =
                mdb_cursor_get(cursor_, k, &v, static_cast<MDB_cursor_op>(op));
            ec) {
      case MDB_SUCCESS:
        return std::make_pair(from_mdb_val(*k), from_mdb_val(v));
      case MDB_NOTFOUND: return std::nullopt;
      default: throw std::system_error{error::make_error_code(ec)};
    }
  }

  template <typename T>
  void put(T key, std::string_view value,
           put_flags const flags = put_flags::NONE) {
    auto k = to_mdb_val(key);
    auto v = to_mdb_val(value);
    EX(mdb_cursor_put(cursor_, &k, &v, static_cast<unsigned>(flags)));
  }

  void del() { EX(mdb_cursor_del(cursor_, 0)); }

  void del_nodupdata() { EX(mdb_cursor_del(cursor_, MDB_NODUPDATA)); }

  mdb_size_t count() {
    mdb_size_t n;
    EX(mdb_cursor_count(cursor_, &n));
    return n;
  }

  txn::dbi get_dbi() {
    return {mdb_cursor_txn(cursor_), mdb_cursor_dbi(cursor_)};
  }

  MDB_cursor* cursor_;
};

}  // namespace lmdb