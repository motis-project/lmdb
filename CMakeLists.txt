project(lmdb)
cmake_minimum_required(VERSION 3.9)

option(LMDB_LINT "Run clang-tidy with the compiler." OFF)
if(LMDB_LINT)
  include(cmake/clang-tidy.cmake)
endif()

include(cmake/pkg.cmake)
find_package(Threads)

file(GLOB_RECURSE lmdb-lib-files lib/*.c)
if (MSVC)
  set_source_files_properties(${lmdb-lib-files} PROPERTIES COMPILE_FLAGS /w)
endif()

file(GLOB_RECURSE lmdb-src-files src/*.cc)
add_library(lmdb STATIC "${lmdb-lib-files}" "${lmdb-src-files}")
target_compile_definitions(lmdb PRIVATE MDB_USE_ROBUST=0)
target_link_libraries(lmdb ${CMAKE_THREAD_LIBS_INIT})
if (MSVC)
  target_link_libraries(lmdb ntdll)
endif()
target_include_directories(lmdb PUBLIC include)
target_include_directories(lmdb SYSTEM PUBLIC lib)
target_compile_features(lmdb PUBLIC cxx_std_17)

file(GLOB_RECURSE lmdb-test-files test/*.cc)
add_executable(lmdb-test ${lmdb-test-files})
target_link_libraries(lmdb-test lmdb doctest)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_options(lmdb-test PRIVATE
    -fno-strict-aliasing
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-newline-eof
    -Wno-missing-prototypes
    -Wno-padded
    -Wno-double-promotion
    -Wno-undef
    -Wno-undefined-reinterpret-cast
    -Wno-float-conversion
    -Wno-global-constructors
    -Wno-exit-time-destructors
    -Werror
  )
elseif(NOT MSVC)
  target_compile_options(lmdb-test PRIVATE
    -Wall
    -Wextra
    -Werror
  )
endif()
