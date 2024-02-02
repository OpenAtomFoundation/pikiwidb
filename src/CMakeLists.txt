
AUX_SOURCE_DIRECTORY(. PIKIWIDB_SRC)

ADD_EXECUTABLE(pikiwidb ${PIKIWIDB_SRC})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

TARGET_INCLUDE_DIRECTORIES(pikiwidb
        PRIVATE ${PROJECT_SOURCE_DIR}/src
        PRIVATE ${PROJECT_SOURCE_DIR}/src/pstd
        PRIVATE ${PROJECT_SOURCE_DIR}/src/net
        PRIVATE ${PROJECT_SOURCE_DIR}/src/storage/include
        PRIVATE ${rocksdb_SOURCE_DIR}/
        PRIVATE ${rocksdb_SOURCE_DIR}/include
        PRIVATE ${BRAFT_INCLUDE_DIR}
        PRIVATE ${BRPC_INCLUDE_DIR}
        PRIVATE ${GFLAGS_INCLUDE_PATH}
)

TARGET_LINK_LIBRARIES(pikiwidb net; dl; fmt; pikiwidb-folly; storage; pstd braft brpc ssl crypto zlib protobuf leveldb gflags glog rocksdb rt ssl crypto dl z)

SET_TARGET_PROPERTIES(pikiwidb PROPERTIES LINKER_LANGUAGE CXX)
