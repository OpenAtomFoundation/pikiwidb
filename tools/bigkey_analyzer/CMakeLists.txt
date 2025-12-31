cmake_minimum_required(VERSION 3.18)

project(bigkey_analyzer)

set(CMAKE_CXX_STANDARD 17)

# Add the bigkey_analyzer executable
add_executable(bigkey_analyzer
  bigkey_analyzer.cc
)

# Include directories
target_include_directories(bigkey_analyzer PRIVATE
  ${PROJECT_SOURCE_DIR}/../../include
  ${PROJECT_SOURCE_DIR}/../../src
  ${ROCKSDB_INCLUDE_DIR}
  ${INSTALL_INCLUDEDIR}
)

# Link directories
target_link_directories(bigkey_analyzer PRIVATE
  ${INSTALL_LIBDIR_64}
  ${INSTALL_LIBDIR}
)

# Link libraries
target_link_libraries(bigkey_analyzer
  storage
  ${ROCKSDB_LIBRARY}
  ${GLOG_LIBRARY}
  ${LIB_GFLAGS}
  ${LIB_FMT}
  libsnappy.a
  libzstd.a
  liblz4.a
  libz.a
  ${LIBUNWIND_LIBRARY}
  ${JEMALLOC_LIBRARY}
)

# Add dependencies (only if these targets exist in the parent CMake)
if(TARGET storage)
  add_dependencies(bigkey_analyzer storage)
endif()

# Installation
install(TARGETS bigkey_analyzer
  RUNTIME DESTINATION bin
)
