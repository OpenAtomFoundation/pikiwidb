file(GLOB_RECURSE EXCHANGE_SRC CONFIGURE_DEPENDS "*.cpp")
set(EXCHANGE_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/exchange.cpp")
if (EXISTS "${EXCHANGE_MAIN}")
  list(FILTER EXCHANGE_SRC EXCLUDE REGEX ".*/exchange\\.cpp$")
endif()

add_library(exchange_lib STATIC ${EXCHANGE_SRC})

target_include_directories(exchange_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PROJECT_ROOT_DIR}
  ${PROJECT_ROOT_DIR}/include
  ${PROJECT_ROOT_DIR}/src
  ${PROJECT_ROOT_DIR}/src/pstd
  ${PROJECT_ROOT_DIR}/src/pstd/include
  ${PROJECT_ROOT_DIR}/src/storage
  ${PROJECT_ROOT_DIR}/src/storage/include
  ${ROCKSDB_SOURCE_DIR}
)

link_pikiwidb_libs(exchange_lib)

target_link_libraries(exchange_lib PUBLIC pthread z dl)

set_common_definitions(exchange_lib)


if(EXISTS "${EXCHANGE_MAIN}")
  add_executable(exchange "${EXCHANGE_MAIN}")
  target_link_libraries(exchange PRIVATE
    -Wl,--start-group
    exchange_lib
    ${PIKIWIDB_LIBS}
    -Wl,--end-group
    pthread dl
  )
  set_common_definitions(exchange)
endif()

message(STATUS "PROJECT_ROOT_DIR=${PROJECT_ROOT_DIR}")
message(STATUS "PROJECT_DIR=${PROJECT_DIR}")
message(STATUS "CONFIG_DIR=${CONFIG_DIR}")
