#include "lmdb/error.h"

namespace lmdb {

const char* error_category_impl::name() const noexcept { return "lmdb"; }

std::string error_category_impl::message(int ec) const noexcept {
  return mdb_strerror(ec);
}

}  // namespace  lmdb