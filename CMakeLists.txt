project(lmdb)
cmake_minimum_required(VERSION 3.9)

find_package(Threads)

include_directories(include)
include_directories(SYSTEM lib)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(lmdb-compile-flags "\
		-fno-strict-aliasing \
		-Weverything \
		-Wno-c++98-compat \
		-Wno-c++98-compat-pedantic \
		-Wno-newline-eof \
		-Wno-missing-prototypes \
		-Wno-padded \
		-Wno-double-promotion \
		-Wno-undef \
		-Wno-undefined-reinterpret-cast \
		-Wno-float-conversion \
		-Wno-global-constructors \
		-Wno-exit-time-destructors \
		-Werror \
  ")
elseif(NOT MSVC)
  set(lmdb-compile-flags "\
		-Wall \
		-Wextra \
		-Werror \
  ")
endif()

file(GLOB_RECURSE lmdb-lib-files lib/*.c)
file(GLOB_RECURSE lmdb-src-files src/*.cc)
add_library(lmdb STATIC "${lmdb-lib-files}" "${lmdb-src-files}")
target_link_libraries(lmdb ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET lmdb PROPERTY CXX_STANDARD 17)

file(GLOB_RECURSE lmdb-test-files test/*.cc)
add_executable(lmdb-test ${lmdb-test-files})
target_link_libraries(lmdb-test lmdb)
set_property(TARGET lmdb-test PROPERTY CXX_STANDARD 17)
set_target_properties(lmdb-test PROPERTIES COMPILE_FLAGS "${lmdb-compile-flags}")
