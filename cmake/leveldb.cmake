FetchContent_Declare(
    leveldb
    GIT_REPOSITORY https://github.com/google/leveldb.git
    GIT_TAG main
)

# 检查 leveldb 是否已经被下载
FetchContent_GetProperties(leveldb)
if(NOT leveldb_POPULATED)
    FetchContent_Populate(leveldb)
    SET(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    SET(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    SET(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${leveldb_SOURCE_DIR} ${leveldb_BINARY_DIR})
endif()

SET(LEVELDB_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-build/include)
SET(LEVELDB_LIB ${CMAKE_CURRENT_BINARY_DIR}/_deps/leveldb-build/libleveldb.a)