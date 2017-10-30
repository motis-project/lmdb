#pragma once

#include <string>
#include <system_error>
#include <type_traits>

#include "lmdb/lmdb.h"

namespace lmdb {

class error_category_impl : public std::error_category {
 public:
  const char* name() const noexcept override { return "lmdb"; }
  std::string message(int ec) const noexcept override {
    return mdb_strerror(ec);
  }
};

inline const std::error_category& error_category() {
  static error_category_impl instance;
  return instance;
}

namespace error {
inline std::error_code make_error_code(int e) noexcept {
  return std::error_code(e, error_category());
}
}  // namespace error

}  // namespace  lmdb