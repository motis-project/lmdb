project(lmdb)
cmake_minimum_required(VERSION 3.9)

find_package(Threads)

include_directories(include)
include_directories(include/lmdb)

file(GLOB_RECURSE lmdb-files src/*.c)
add_library(lmdb STATIC ${lmdb-files})
if (MSVC)
	target_link_libraries(lmdb ${CMAKE_THREAD_LIBS_INIT} ntdll)
else()
	target_link_libraries(lmdb ${CMAKE_THREAD_LIBS_INIT})
endif()
set_property(TARGET lmdb PROPERTY CXX_STANDARD 17)

file(GLOB_RECURSE lmdb-test-files test/*.cc)
add_executable(lmdb-test ${lmdb-test-files})
target_link_libraries(lmdb-test lmdb)
set_property(TARGET lmdb-test PROPERTY CXX_STANDARD 17)
