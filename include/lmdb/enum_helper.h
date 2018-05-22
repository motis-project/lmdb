// by Mahmoud Al-Qudsi
// Source: https://softwareengineering.stackexchange.com/a/329443

#define ENUM_FLAG_OPERATOR(T, X)                                              \
  inline T operator X(T lhs, T rhs) {                                         \
    return T(static_cast<                                                     \
             std::underlying_type_t<T /* NOLINT(misc-macro-parentheses) */>>( \
        lhs)                                                                  \
                 X /* NOLINT(misc-macro-parentheses) */ static_cast<          \
                     std::underlying_type_t<                                  \
                         T /* NOLINT(misc-macro-parentheses) */>>(rhs));      \
  }

// NOLINTNEXTLINE(misc-macro-parentheses)
#define ENUM_FLAGS(T)                                                         \
  enum class T /* NOLINT(misc-macro-parentheses) */ : uint32_t;               \
  inline T /* NOLINT(misc-macro-parentheses) */ operator~(T t) {              \
    return T(~static_cast<                                                    \
             std::underlying_type_t<T /* NOLINT(misc-macro-parentheses) */>>( \
        t));                                                                  \
  }                                                                           \
  ENUM_FLAG_OPERATOR(T, |)                                                    \
  ENUM_FLAG_OPERATOR(T, ^)                                                    \
  ENUM_FLAG_OPERATOR(T, &)                                                    \
  enum class T /* NOLINT(misc-macro-parentheses) */ : uint32_t
