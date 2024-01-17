# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

set(Protobuf_USE_STATIC_LIBS ON)
set(Protobuf_MSVC_STATIC_RUNTIME OFF)

FetchContent_Declare(
  protocolbuffers_protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf
  GIT_TAG        v3.15.8
)
FetchContent_MakeAvailable(protocolbuffers_protobuf)

set(Protobuf_ROOT ${protocolbuffers_protobuf_SOURCE_DIR}/cmake)
# set(Protobuf_DIR ${Protobuf_ROOT}/${CMAKE_INSTALL_LIBDIR}/cmake/protobuf)

message(STATUS "Setting up protobuf ...")
execute_process(
  COMMAND
    ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -D protobuf_BUILD_TESTS=OFF -D protobuf_BUILD_PROTOC_BINARIES=ON -D CMAKE_POSITION_INDEPENDENT_CODE=ON -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${Protobuf_ROOT})
if(result)
  message(FATAL_ERROR "Failed to download protobuf (${result})!")
endif()

message(STATUS "Building protobuf ...")
execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${Protobuf_ROOT})
if(result)
  message(FATAL_ERROR "Failed to build protobuf (${result})!")
endif()

message(STATUS "Installing protobuf ...")

set(Protobuf_INCLUDE_DIR ${protocolbuffers_protobuf_SOURCE_DIR}/src)
set(Protobuf_LIBRARIES ${Protobuf_ROOT})
set(PROTOC_LIB ${Protobuf_ROOT})
set(PROTOBUF_PROTOC "${Protobuf_ROOT}/protoc" CACHE FILEPATH "Path to protoc executable" FORCE)
set(PROTOBUF_PROTOC_EXECUTABLE "${Protobuf_ROOT}/protoc" CACHE FILEPATH "Path to protoc executable" FORCE)
# set(PROTOBUF_PROTOC_EXECUTABLE "${Protobuf_ROOT}/protoc")

# set(Protobuf_VERSION 3.15.8.0)
# find_package(Protobuf REQUIRED HINTS ${Protobuf_DIR})

# include(${Protobuf_DIR}/protobuf-config.cmake)
# include(${Protobuf_DIR}/protobuf-module.cmake)
# include(${Protobuf_DIR}/protobuf-options.cmake)
# include(${Protobuf_DIR}/protobuf-targets.cmake)

# if(Protobuf_FOUND)
#   message(STATUS "Protobuf version : ${Protobuf_VERSION}")
#   message(STATUS "Protobuf include path : ${Protobuf_INCLUDE_DIRS}")
#   message(STATUS "Protobuf libraries : ${Protobuf_LIBRARIES}")
#   message(STATUS "Protobuf compiler libraries : ${Protobuf_PROTOC_LIBRARIES}")
#   message(STATUS "Protobuf lite libraries : ${Protobuf_LITE_LIBRARIES}")
#   message(STATUS "Protobuf protoc : ${Protobuf_PROTOC_EXECUTABLE}")
# else()
#   message(
#     WARNING
#       "Protobuf package not found -> specify search path via Protobuf_ROOT variable"
#   )
# endif()

# include_directories(${Protobuf_INCLUDE_DIRS})
