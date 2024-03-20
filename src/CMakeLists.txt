
AUX_SOURCE_DIRECTORY(. PIKIWIDB_SRC)

ADD_EXECUTABLE(pikiwidb ${PIKIWIDB_SRC} checkpoint_manager.h)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

TARGET_INCLUDE_DIRECTORIES(pikiwidb PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/pstd
        ${PROJECT_SOURCE_DIR}/src/net
        ${PROJECT_SOURCE_DIR}/src/storage/include
        ${rocksdb_SOURCE_DIR}/
        ${rocksdb_SOURCE_DIR}/include
)

TARGET_LINK_LIBRARIES(pikiwidb net; dl; fmt; storage; rocksdb)
SET_TARGET_PROPERTIES(pikiwidb PROPERTIES LINKER_LANGUAGE CXX)
