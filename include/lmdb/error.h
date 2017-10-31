#pragma once

#include <string>
#include <system_error>

#include "lmdb.h"

namespace lmdb {

class error_category_impl : public std::error_category {
public:
  const char* name() const noexcept override;
  std::string message(int ec) const noexcept override;
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