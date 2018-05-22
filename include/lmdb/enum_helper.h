// by Mahmoud Al-Qudsi
// Source: https://softwareengineering.stackexchange.com/a/329443

// NOLINTNEXTLINE(misc-macro-parentheses)
#define ENUM_FLAG_OPERATOR(T, X)                                 \
  inline T operator X(T lhs, T rhs) {                            \
    return T(static_cast<std::underlying_type_t<T>>(lhs)         \
                 X static_cast<std::underlying_type_t<T>>(rhs)); \
  }

// NOLINTNEXTLINE(misc-macro-parentheses)
#define ENUM_FLAGS(T)                                     \
  enum class T : uint32_t;                                \
  inline T operator~(T t) {                               \
    return T(~static_cast<std::underlying_type_t<T>>(t)); \
  }                                                       \
  ENUM_FLAG_OPERATOR(T, |)                                \
  ENUM_FLAG_OPERATOR(T, ^)                                \
  ENUM_FLAG_OPERATOR(T, &)                                \
  enum class T : uint32_t
