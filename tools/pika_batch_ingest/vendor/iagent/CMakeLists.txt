file(GLOB_RECURSE IAGENT_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

set(IAGENT_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/iagent.cpp")
if(EXISTS "${IAGENT_MAIN}")
  list(FILTER IAGENT_SRC EXCLUDE REGEX ".*/iagent\\.cpp$")
endif()

set(HIREDIS_ROOT ${INGEST_ROOT_DIR}/third/hiredis)

find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h
  HINTS
    ${HIREDIS_ROOT}/include
    ${PROJECT_ROOT_DIR}/deps/include
    ${CMAKE_SOURCE_DIR}/deps/include
    /usr/local/include /usr/include
)

find_library(HIREDIS_LIBRARY NAMES hiredis
  HINTS
    ${HIREDIS_ROOT}/lib      ${HIREDIS_ROOT}/lib64
    ${PROJECT_ROOT_DIR}/deps/lib   ${PROJECT_ROOT_DIR}/deps/lib64
    ${CMAKE_SOURCE_DIR}/deps/lib
    /usr/local/lib /usr/lib
)

add_library(hiredis::hiredis STATIC IMPORTED GLOBAL)
set_target_properties(hiredis::hiredis PROPERTIES
  IMPORTED_LOCATION "${HIREDIS_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${HIREDIS_INCLUDE_DIR}"
)

if(IAGENT_SRC)
  add_library(iagent_lib STATIC ${IAGENT_SRC})
  target_include_directories(iagent_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HIREDIS_INCLUDE_DIR}
  )
  target_link_libraries(iagent_lib PUBLIC hiredis::hiredis)
else()
  add_library(iagent_lib INTERFACE)
  target_include_directories(iagent_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${HIREDIS_INCLUDE_DIR}
  )
  target_link_libraries(iagent_lib INTERFACE hiredis::hiredis)
endif()

link_pikiwidb_libs(iagent_lib)
link_aws_static(iagent_lib)

if(TARGET iagent_lib AND TARGET_PROPERTY_VALUE)
endif()
target_link_libraries(iagent_lib PUBLIC pthread dl z)

set_common_definitions(iagent_lib)

if(EXISTS "${IAGENT_MAIN}")
  add_executable(iagent "${IAGENT_MAIN}")
  target_link_libraries(iagent PRIVATE iagent_lib)
  set_common_definitions(iagent)
endif()

list(LENGTH IAGENT_SRC _n)
message(STATUS "IAGENT_SRC count = ${_n}")
message(STATUS "HIREDIS_INCLUDE_DIR = ${HIREDIS_INCLUDE_DIR}")
message(STATUS "HIREDIS_LIBRARY     = ${HIREDIS_LIBRARY}")
