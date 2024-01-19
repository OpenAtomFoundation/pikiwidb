ADD_EXECUTABLE(hash_test ${CMAKE_CURRENT_SOURCE_DIR}/hash_test.cc)

TARGET_INCLUDE_DIRECTORIES(hash_test 
  PUBLIC ${CMAKE_SOURCE_DIR}/src/storage/include
  PRIVATE ${rocksdb_SOURCE_DIR}/include
)

TARGET_LINK_LIBRARIES(hash_test gtest gtest_main storage rocksdb)
