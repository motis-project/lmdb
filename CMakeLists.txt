project(lmdb)
cmake_minimum_required(VERSION 3.8)

include_directories(include/lmdb)

file(GLOB_RECURSE lmdb-files src/*.c)
add_library(lmdb STATIC ${lmdb-files})

file(GLOB_RECURSE lmdb-test-files test/*.cc)
add_executable(lmdb-test ${lmdb-test-files})
target_link_libraries(lmdb-test lmdb)