cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE TEST_SRC CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE MOCK_TEST_SRC CONFIGURE_DEPENDS "mock/*.cpp")
file(GLOB_RECURSE IAGENT_TEST_SRC CONFIGURE_DEPENDS "iagent/*.cpp")

list(REMOVE_ITEM TEST_SRC ${MOCK_TEST_SRC})
list(REMOVE_ITEM TEST_SRC ${IAGENT_TEST_SRC})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_bingest ${TEST_SRC})

file(GLOB S3PUT_PB_SRCS "${CMAKE_SOURCE_DIR}/src/s3put/include/proto/*.pb.cc")
if(S3PUT_PB_SRCS)
  message(STATUS "test_bingest: add proto sources: ${S3PUT_PB_SRCS}")
  target_sources(test_bingest PRIVATE ${S3PUT_PB_SRCS})
endif()

target_include_directories(test_bingest PUBLIC
  ${CMAKE_SOURCE_DIR}/test
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/mock/include
  ${CMAKE_SOURCE_DIR}/src/exchange/include
  ${CMAKE_SOURCE_DIR}/src/s3put/include
  ${CMAKE_SOURCE_DIR}/src/s3put/include/proto
  ${CMAKE_SOURCE_DIR}/vendor/iagent/include
  ${OPENSSL_INCLUDE_DIR}
  ${CURL_INCLUDE_DIR}
  ${AWS_CRT_CPP_DIR}/include
  ${AWS_SDK_CPP_DIR}/include
  ${ROCKSDB_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}/src/s3put
)

target_link_libraries(test_bingest PRIVATE
  "-Wl,--start-group"
    exchange_lib
    mock_lib
    s3put_lib
    pthread
    z
  "-Wl,--end-group"
)

link_pikiwidb_libs(test_bingest)
link_aws_static(test_bingest)

target_compile_definitions(test_bingest PRIVATE
  PROJECT_DIR="${CMAKE_SOURCE_DIR}/data/test"
  DEBUG=1
)

enable_testing()
add_test(NAME test_bingest COMMAND test_bingest)

message(STATUS "PROJECT_DIR=${PROJECT_DIR}")
message(STATUS "CONFIG_DIR=${CONFIG_DIR}")
