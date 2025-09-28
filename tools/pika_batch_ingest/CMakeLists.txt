cmake_minimum_required(VERSION 3.15)
project(bingest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

cmake_policy(SET CMP0079 NEW)

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}/data")
set(CONFIG_DIR  "${CMAKE_SOURCE_DIR}/config")

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third/ThreadPool
)

set(DEPS_INCLUDE_DIR "${PROJECT_ROOT_DIR}/deps/include")
if(EXISTS "${DEPS_INCLUDE_DIR}")
  include_directories(${DEPS_INCLUDE_DIR})
endif()

set(INGEST_ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(EP_BASE_SUFFIX "buildtrees")
set(STAGED_INSTALL_PREFIX ${PROJECT_ROOT_DIR}/deps)
set(INSTALL_INCLUDEDIR ${STAGED_INSTALL_PREFIX}/include)
set(ROCKSDB_INCLUDE_DIR ${INSTALL_INCLUDEDIR})
set(ROCKSDB_SOURCE_DIR ${PROJECT_ROOT_DIR}/${EP_BASE_SUFFIX}/Source/rocksdb)

set(PIKIWIDB_LIB_PATH "${CMAKE_SOURCE_DIR}/vendor/lib/pika")
message(STATUS "Using link directories: ${PIKIWIDB_LIB_PATH}")

set(_is_debug OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_is_debug ON)
endif()

macro(_pick_lib outvar)
  set(_cands ${ARGN})
  unset(${outvar})
  foreach(_c IN LISTS _cands)
    if(_is_debug)
      if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}d.a")
        set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}d.a")
        break()
      endif()
      if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}_debug.a")
        set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}_debug.a")
        break()
      endif()
      if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}_nothreads_debug.a")
        set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}_nothreads_debug.a")
        break()
      endif()
    endif()
    if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}.a")
      set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}.a")
      break()
    endif()
    if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}_nothreads.a")
      set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}_nothreads.a")
      break()
    endif()
    if(EXISTS "${PIKIWIDB_LIB_PATH}/lib${_c}_static.a")
      set(${outvar} "${PIKIWIDB_LIB_PATH}/lib${_c}_static.a")
      break()
    endif()
  endforeach()
  if(NOT ${outvar})
    message(FATAL_ERROR "[FAIL] Required library not found in ${PIKIWIDB_LIB_PATH}: one of [${_cands}]")
  endif()
endmacro()

# =========================
# Pikiwidb
# =========================
_pick_lib(PIKI_LIB_pstd       pstd)
_pick_lib(PIKI_LIB_net        net)
_pick_lib(PIKI_LIB_storage    storage)
_pick_lib(PIKI_LIB_cache      cache)
_pick_lib(PIKI_LIB_rediscache rediscache)
_pick_lib(PIKI_LIB_glog       glog glogd glog_debug)
_pick_lib(PIKI_LIB_protobuf   protobuf protobuf-lite protobufd protobuf-lited)
_pick_lib(PIKI_LIB_gflags
  gflags gflags_nothreads
  gflags_debug gflags_nothreads_debug
  gflagsd gflags_nothreadsd
)
_pick_lib(PIKI_LIB_z          z)
_pick_lib(PIKI_LIB_snappy     snappy)
_pick_lib(PIKI_LIB_zstd       zstd)
_pick_lib(PIKI_LIB_lz4        lz4)
_pick_lib(PIKI_LIB_unwind     unwind)

_pick_lib(PIKI_LIB_ssl       ssl)
_pick_lib(PIKI_LIB_crypto    crypto)
_pick_lib(PIKI_LIB_curl      curl)
_pick_lib(PIKI_LIB_rocksdb   rocksdb)

_pick_lib(PIKI_LIB_gtest       gtest)
_pick_lib(PIKI_LIB_gtest_main  gtest_main)
_pick_lib(PIKI_LIB_gmock       gmock)
_pick_lib(PIKI_LIB_gmock_main  gmock_main)

set(PIKIWIDB_LIBS
  ${PIKI_LIB_pstd}
  ${PIKI_LIB_net}
  ${PIKI_LIB_storage}
  ${PIKI_LIB_cache}
  ${PIKI_LIB_rediscache}
  ${PIKI_LIB_glog}
  ${PIKI_LIB_protobuf}
  ${PIKI_LIB_gflags}
  ${PIKI_LIB_z}
  ${PIKI_LIB_snappy}
  ${PIKI_LIB_zstd}
  ${PIKI_LIB_lz4}
  ${PIKI_LIB_unwind}
  ${PIKI_LIB_rocksdb}
  ${PIKI_LIB_gtest}
  ${PIKI_LIB_gtest_main}
  ${PIKI_LIB_gmock}
  ${PIKI_LIB_gmock_main}
)

function(link_pikiwidb_libs target)
  target_link_libraries(${target} PUBLIC ${PIKIWIDB_LIBS})
endfunction()

function(set_common_definitions target)
  target_compile_definitions(${target} PUBLIC
    PROJECT_DIR="${PROJECT_DIR}"
    CONFIG_DIR="${CONFIG_DIR}"
  )
endfunction()

set(OPENSSL_INCLUDE_DIR "${PROJECT_ROOT_DIR}/deps/include" CACHE PATH "")
set(CURL_INCLUDE_DIR    "${PROJECT_ROOT_DIR}/deps/include" CACHE PATH "")
set(CURL_INCLUDE_DIR    "${PROJECT_ROOT_DIR}/deps/include" CACHE PATH "")

set(AWS_CRT_CPP_DIR "${PROJECT_ROOT_DIR}/deps" CACHE PATH "")
set(AWS_SDK_CPP_DIR "${PROJECT_ROOT_DIR}/deps" CACHE PATH "")

include_directories(${OPENSSL_INCLUDE_DIR} ${CURL_INCLUDE_DIR})
include_directories(${AWS_CRT_CPP_DIR}/include ${AWS_SDK_CPP_DIR}/include)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")

include(${CMAKE_CURRENT_LIST_DIR}/aws.cmake)

add_subdirectory(src/mock)
add_subdirectory(src/exchange)
add_subdirectory(src/s3put)
add_subdirectory(vendor/iagent)
add_subdirectory(test)
add_subdirectory(test/mock)
add_subdirectory(test/iagent)
