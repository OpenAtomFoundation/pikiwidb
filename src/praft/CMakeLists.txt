cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)
project(praft)

# Generate binlog protobuf files
set(BINLOG_PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/binlog.proto)
custom_protobuf_generate_cpp(BINLOG_PROTO_SRCS BINLOG_PROTO_HDRS ${BINLOG_PROTO_FILE})
message("praft BINLOG_PROTO_SRCS = ${BINLOG_PROTO_SRCS}")
message("praft BINLOG_PROTO_HDRS = ${BINLOG_PROTO_HDRS}")

# Create a separate binlog_pb library for reuse
add_library(binlog_pb STATIC ${BINLOG_PROTO_SRCS})
target_include_directories(binlog_pb
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${INSTALL_INCLUDEDIR}
)
target_link_libraries(binlog_pb PUBLIC ${PROTOBUF_LIBRARY})
add_dependencies(binlog_pb protobuf)

# Collect all source files (excluding binlog.proto which is in binlog_pb)
aux_source_directory(./src DIR_SRCS)

# Create static library (WITHOUT binlog protobuf sources, link binlog_pb instead)
add_library(praft STATIC ${DIR_SRCS})

# Dependencies
add_dependencies(praft 
    gflags
    protobuf
    leveldb
    brpc
    braft
    glog
    pstd
    storage
    binlog_pb
    pika_proto_gen
)

# Include directories
target_include_directories(praft
    PUBLIC ${PROJECT_SOURCE_DIR}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${CMAKE_SOURCE_DIR}  # Project root for include/pika_*.h
    PUBLIC ${CMAKE_SOURCE_DIR}/src  # For other modules
    PUBLIC ${CMAKE_SOURCE_DIR}/src/storage/include  # For storage module
    PUBLIC ${CMAKE_SOURCE_DIR}/src/pstd/include  # For pstd module
    PUBLIC ${CMAKE_BINARY_DIR}  # For generated protobuf files (pika_inner_message.pb.h)
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}  # For generated binlog.pb.h
    PUBLIC ${INSTALL_INCLUDEDIR}
)

# Link libraries
target_link_libraries(praft
    PUBLIC binlog_pb  # Link the binlog protobuf library
    PUBLIC ${BRAFT_LIBRARY}
    PUBLIC ${BRPC_LIBRARY}
    PUBLIC ${LEVELDB_LIBRARY}
    PUBLIC ${PROTOBUF_LIBRARY}
    PUBLIC ${GFLAGS_LIBRARY}
    PUBLIC ${GLOG_LIBRARY}
    PUBLIC pstd
    PUBLIC storage
)

# Platform-specific libraries
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # macOS frameworks
    target_link_libraries(praft
        PUBLIC "-framework CoreFoundation"
        PUBLIC "-framework CoreGraphics"
        PUBLIC "-framework CoreData"
        PUBLIC "-framework CoreText"
        PUBLIC "-framework Security"
        PUBLIC "-framework Foundation"
        PUBLIC "-framework ApplicationServices"
        PUBLIC "-framework SystemConfiguration"
        PUBLIC "-framework AppKit"
    )
    
    # OpenSSL
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        target_include_directories(praft PUBLIC ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(praft PUBLIC ${OPENSSL_LIBRARIES})
    else()
        find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto libcrypto)
        find_library(OPENSSL_SSL_LIBRARY NAMES ssl libssl)
        if(OPENSSL_CRYPTO_LIBRARY)
            target_link_libraries(praft PUBLIC ${OPENSSL_CRYPTO_LIBRARY})
        endif()
        if(OPENSSL_SSL_LIBRARY)
            target_link_libraries(praft PUBLIC ${OPENSSL_SSL_LIBRARY})
        endif()
    endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Linux system libraries
    target_link_libraries(praft
        PUBLIC rt
        PUBLIC dl
    )
    
    # OpenSSL
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        target_link_libraries(praft PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    else()
        find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto libcrypto)
        find_library(OPENSSL_SSL_LIBRARY NAMES ssl libssl)
        if(OPENSSL_CRYPTO_LIBRARY AND OPENSSL_SSL_LIBRARY)
            target_link_libraries(praft PUBLIC ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
        endif()
    endif()
endif()

