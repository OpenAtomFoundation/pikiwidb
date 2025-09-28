cmake_minimum_required(VERSION 3.18)

project(ingest)
set(CMAKE_CXX_STANDARD 17)

file(GLOB INGEST_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_library(ingest STATIC ${INGEST_SRCS})

add_definitions(-DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX)
add_compile_options(-fno-builtin-memcmp)
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
  add_compile_options(-msse)
endif()

set(PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/manifest.proto)
custom_protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
add_library(ingest_protos STATIC ${PROTO_SRCS})
add_dependencies(ingest_protos protobuf)

add_library(protobuf_static STATIC IMPORTED GLOBAL)
set_target_properties(protobuf_static PROPERTIES
  IMPORTED_LOCATION             "${PROTOBUF_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}"
)
add_dependencies(protobuf_static protobuf)

target_link_libraries(ingest_protos PUBLIC protobuf_static)

target_include_directories(ingest_protos
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIR}
)

target_include_directories(ingest
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIR}
    ${INSTALL_INCLUDEDIR}
    ${ROCKSDB_SOURCE_DIR}
)

add_dependencies(ingest ingest_protos)

target_link_libraries(ingest
  PUBLIC
    ingest_protos
    pstd
    storage
    net
    cache
    ${ROCKSDB_LIBRARY}
    ${SNAPPY_LIBRARY}
    ${ZSTD_LIBRARY}
    ${LZ4_LIBRARY}
    ${ZLIB_LIBRARY}
    ${JEMALLOC_LIBRARY}
    ${GLOG_LIBRARY}
    ${GFLAGS_LIBRARY}
    ${FMT_LIBRARY}
    ${LIBUNWIND_LIBRARY}
    aws-cpp-sdk-transfer
    aws-cpp-sdk-s3
    aws-cpp-sdk-core
    aws-crt-cpp
    aws-c-s3
    aws-c-mqtt
    aws-c-auth
    aws-c-http
    aws-c-io
    aws-c-compression
    aws-c-cal
    aws-c-sdkutils
    aws-c-event-stream
    aws-c-common
    aws-checksums
    s2n
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    dl
)

add_subdirectory(tests)
