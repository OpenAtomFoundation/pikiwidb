# SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# # option(EXAMPLE_LINK_SO "Whether examples are linked dynamically" OFF)
# # option(LINK_TCMALLOC "Link tcmalloc if possible" ON)

# execute_process(
#     COMMAND bash -c "find ${CMAKE_SOURCE_DIR}/../.. -type d -path \"*output/include/braft\" | xargs dirname | xargs dirname | tr -d '\n'"
#     OUTPUT_VARIABLE OUTPUT_PATH
# )

# set(CMAKE_PREFIX_PATH ${OUTPUT_PATH})

# include(FindThreads)
# include(FindProtobuf)
# message(STATUS ${PROTOBUF_PROTOC_EXECUTABLE})
# if (NOT PROTOBUF_PROTOC_EXECUTABLE)
#     get_filename_component(PROTO_LIB_DIR ${PROTOBUF_LIBRARY} DIRECTORY)
#     set (PROTOBUF_PROTOC_EXECUTABLE "${PROTO_LIB_DIR}/../bin/protoc")
# endif()

# # protobuf_generate_cpp(PROTO_SRC PROTO_HEADER counter.proto)
# file(GLOB PROTO_FILES ${CMAKE_CURRENT_BINARY_DIR}/*.proto)
# message("protoc: ${PROTOBUF_PROTOC_EXECUTABLE}, proto inc: ${PROTOBUF_INCLUDE_DIRS}, lib: ${PROTOBUF_LIBRARIES}, ${PROTOBUF_PROTOC_LIBRARY}, protos: ${PROTO_FILES}")
# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
# foreach (PROTO ${PROTO_FILES})
#     message(proto : ${PROTO})
#     get_filename_component(PROTO_WE ${PROTO} NAME)
#     string(REPLACE ".proto" "" PROTO_WE ${PROTO_WE})
#     list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.h")
#     list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc")
#     add_custom_command(
#             OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.h ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_WE}.pb.cc
#             COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
#             --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
#             --proto_path=${PROTOBUF_INCLUDE_DIR}
#             --proto_path=${CMAKE_CURRENT_BINARY_DIR} ${PROTO}
#             DEPENDS protobuf
#     )
# endforeach ()

# # include PROTO_HEADER
# message("${CMAKE_CURRENT_BINARY_DIR}")
# include_directories(${CMAKE_CURRENT_BINARY_DIR})

# AUX_SOURCE_DIRECTORY(. PRAFT_SRC)
# list(APPEND PRAFT_SRC ${PROTO_HDRS} ${PROTO_SRCS})
# ADD_LIBRARY(praft ${PRAFT_SRC})
# add_dependencies(praft gflags protobuf leveldb braft brpc ssl crypto)

# # find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
# # if(EXAMPLE_LINK_SO)
# #     find_library(BRPC_LIB NAMES brpc)
# #     find_library(BRAFT_LIB NAMES braft)
# # else()
# #     find_library(BRPC_LIB NAMES libbrpc.a brpc)
# #     find_library(BRAFT_LIB NAMES libbraft.a braft)
# # endif()

# # if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
# #     message(FATAL_ERROR "Fail to find brpc")
# # endif()
# include_directories(${BRPC_INCLUDE_DIR})

# # find_path(BRAFT_INCLUDE_PATH NAMES braft/raft.h)
# # if ((NOT BRAFT_INCLUDE_PATH) OR (NOT BRAFT_LIB))
# #     message (FATAL_ERROR "Fail to find braft")
# # endif()
# include_directories(${BRAFT_INCLUDE_DIR})

# # find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
# # find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
# # if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
# #     message(FATAL_ERROR "Fail to find gflags")
# # endif()
# include_directories(${GFLAGS_INCLUDE_PATH})

# execute_process(
#     COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
#     OUTPUT_VARIABLE GFLAGS_NS
# )
# if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
#     execute_process(
#         COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
#         OUTPUT_VARIABLE GFLAGS_NS
#     )
# endif()

# # if (LINK_TCMALLOC)
# #     find_path(GPERFTOOLS_INCLUDE_DIR NAMES gperftools/heap-profiler.h)
# #     find_library(GPERFTOOLS_LIBRARIES NAMES tcmalloc_and_profiler)
# #     if (GPERFTOOLS_INCLUDE_DIR AND GPERFTOOLS_LIBRARIES)
# #         set(CMAKE_CXX_FLAGS "-DBRPC_ENABLE_CPU_PROFILER")
# #         include_directories(${GPERFTOOLS_INCLUDE_DIR})
# #     else ()
# #         set (GPERFTOOLS_LIBRARIES "")
# #     endif ()
# # endif ()

# # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CPP_FLAGS} -DGFLAGS_NS=${GFLAGS_NS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
# # if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# #     # require at least gcc 4.8
# #     if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
# #         message(FATAL_ERROR "GCC is too old, please install a newer version supporting C++11")
# #     endif()
# # elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# #     # require at least clang 3.3
# #     if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
# #         message(FATAL_ERROR "Clang is too old, please install a newer version supporting C++11")
# #     endif()
# # else()
# #     message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
# # endif()

# # if(CMAKE_VERSION VERSION_LESS "3.1.3")
# #     if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# #         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# #     endif()
# #     if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# #         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
# #     endif()
# # else()
# #     set(CMAKE_CXX_STANDARD 11)
# #     set(CMAKE_CXX_STANDARD_REQUIRED ON)
# # endif()

# # find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
# # find_library(LEVELDB_LIB NAMES leveldb)
# # if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
# #     message(FATAL_ERROR "Fail to find leveldb")
# # endif()
# include_directories(${LEVELDB_INCLUDE_PATH})

# include_directories(${CMAKE_SOURCE_DIR}/src)
# include_directories(${PROJECT_SOURCE_DIR}/src/pstd)
# include_directories(${PROJECT_SOURCE_DIR}/src/storage/include)
# include_directories(${PROJECT_SOURCE_DIR}/src/net)
# include_directories(${rocksdb_SOURCE_DIR}/)
# include_directories(${rocksdb_SOURCE_DIR}/include)

# # add_executable(counter_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
# # add_executable(counter_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})


# set(DYNAMIC_LIB
#     ${CMAKE_THREAD_LIBS_INIT}
#     ${GFLAGS_LIB}
#     ${PROTOBUF_LIBRARY}
#     # ${GPERFTOOLS_LIBRARIES}
#     ${LEVELDB_LIB}
#     ${BRAFT_LIBRARIES}
#     ${BRPC_LIBRARIES}
#     pstd
#     storage
#     net
#     rocksdb
#     rt
#     ssl
#     crypto
#     dl
#     z
#     )
    
# # target_link_libraries(counter_client
# #                       "-Xlinker \"-(\""
# #                       ${DYNAMIC_LIB}
# #                       "-Xlinker \"-)\"")
# # target_link_libraries(counter_server
# #                       "-Xlinker \"-(\""
# #                       ${DYNAMIC_LIB}
# #                       "-Xlinker \"-)\"")
# target_link_libraries(praft
#                       "-Xlinker \"-(\""
#                       ${DYNAMIC_LIB}
#                       "-Xlinker \"-)\"")


FILE(GLOB PRAFT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
ADD_LIBRARY(praft ${PRAFT_SRC})

TARGET_INCLUDE_DIRECTORIES(praft
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        PRIVATE ${PROJECT_SOURCE_DIR}/src/pstd
        PRIVATE ${PROJECT_SOURCE_DIR}/src/net
        PRIVATE ${PROJECT_SOURCE_DIR}/src/storage/include
        PRIVATE ${rocksdb_SOURCE_DIR}/
        PRIVATE ${rocksdb_SOURCE_DIR}/include
        PRIVATE ${BRAFT_INCLUDE_DIR}
        PRIVATE ${BRPC_INCLUDE_DIR}
        PRIVATE ${GFLAGS_INCLUDE_PATH}
        PRIVATE ${PROJECT_SOURCE_DIR}/src/praft
)

TARGET_LINK_LIBRARIES(praft net; dl; fmt; pikiwidb-folly; storage; pstd braft brpc ssl crypto zlib protobuf leveldb gflags glog rocksdb rt ssl crypto dl z)

SET_TARGET_PROPERTIES(praft PROPERTIES LINKER_LANGUAGE CXX)
                      

