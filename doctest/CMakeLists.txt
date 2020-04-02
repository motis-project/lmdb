project(doctest)

if (NOT TARGET doctest)
  add_library(doctest INTERFACE)
  target_include_directories(doctest SYSTEM INTERFACE .)
endif()