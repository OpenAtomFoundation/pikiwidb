file(GLOB_RECURSE S3PUT_SRC CONFIGURE_DEPENDS "*.cpp" "*.cc")

file(GLOB S3PUT_PB_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/include/proto/*.pb.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.pb.cc"
)

set(S3PUT_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/s3put.cpp")
if(EXISTS "${S3PUT_MAIN}")
  list(FILTER S3PUT_SRC EXCLUDE REGEX ".*/s3put\\.cpp$")
endif()

add_library(s3put_lib STATIC ${S3PUT_SRC} ${S3PUT_PB_SRCS})

target_include_directories(s3put_lib
  BEFORE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/proto
)

link_pikiwidb_libs(s3put_lib)
link_aws_static(s3put_lib)
target_link_libraries(s3put_lib PUBLIC pthread dl)
set_common_definitions(s3put_lib)

message(STATUS "PROJECT_DIR=${PROJECT_DIR}")
message(STATUS "CONFIG_DIR=${CONFIG_DIR}")

if(EXISTS "${S3PUT_MAIN}")
  add_executable(s3put "${S3PUT_MAIN}")
  target_link_libraries(s3put PRIVATE
    s3put_lib
    ${PIKI_LIB_protobuf}      
  )
  set_common_definitions(s3put)
endif()
