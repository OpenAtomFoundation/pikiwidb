file(GLOB_RECURSE MOCK_SRC CONFIGURE_DEPENDS "*.cpp")
set(MOCK_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/mock.cpp")
if(EXISTS "${MOCK_MAIN}")
  list(FILTER MOCK_SRC EXCLUDE REGEX ".*/mock\\.cpp$")
endif()

add_library(mock_lib STATIC ${MOCK_SRC})

target_include_directories(mock_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_pikiwidb_libs(mock_lib)

target_link_libraries(mock_lib PUBLIC pthread z dl)

set_common_definitions(mock_lib)

message(STATUS "PROJECT_DIR=${PROJECT_DIR}")
message(STATUS "CONFIG_DIR=${CONFIG_DIR}")

if(EXISTS "${MOCK_MAIN}")
  add_executable(mock "${MOCK_MAIN}")
  target_link_libraries(mock PRIVATE mock_lib)
  set_common_definitions(mock)
endif()
