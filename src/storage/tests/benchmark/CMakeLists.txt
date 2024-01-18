ADD_EXECUTABLE(binlog_benchmark ${CMAKE_CURRENT_SOURCE_DIR}/binlog_benchmark.cc)

TARGET_INCLUDE_DIRECTORIES(binlog_benchmark 
  PUBLIC ${CMAKE_SOURCE_DIR}/src/storage/src
  PRIVATE ${rocksdb_SOURCE_DIR}/include
)

TARGET_LINK_LIBRARIES(binlog_benchmark gtest gtest_main storage rocksdb)
