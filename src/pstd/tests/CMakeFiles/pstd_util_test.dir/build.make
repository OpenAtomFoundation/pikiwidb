# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/clion/clion-2023.2.2/bin/cmake/linux/x64/bin/cmake

# The command to remove a file.
RM = /opt/clion/clion-2023.2.2/bin/cmake/linux/x64/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pan/MyRepo/pikiwidb

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pan/MyRepo/pikiwidb

# Include any dependencies generated for this target.
include src/pstd/tests/CMakeFiles/pstd_util_test.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/pstd/tests/CMakeFiles/pstd_util_test.dir/compiler_depend.make

# Include the progress variables for this target.
include src/pstd/tests/CMakeFiles/pstd_util_test.dir/progress.make

# Include the compile flags for this target's objects.
include src/pstd/tests/CMakeFiles/pstd_util_test.dir/flags.make

src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o: src/pstd/tests/CMakeFiles/pstd_util_test.dir/flags.make
src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o: src/pstd/tests/pstd_util_test.cc
src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o: src/pstd/tests/CMakeFiles/pstd_util_test.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o -MF CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o.d -o CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o -c /home/pan/MyRepo/pikiwidb/src/pstd/tests/pstd_util_test.cc

src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/pstd/tests/pstd_util_test.cc > CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.i

src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/pstd/tests/pstd_util_test.cc -o CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.s

# Object files for target pstd_util_test
pstd_util_test_OBJECTS = \
"CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o"

# External object files for target pstd_util_test
pstd_util_test_EXTERNAL_OBJECTS =

src/pstd/tests/pstd_util_test: src/pstd/tests/CMakeFiles/pstd_util_test.dir/pstd_util_test.cc.o
src/pstd/tests/pstd_util_test: src/pstd/tests/CMakeFiles/pstd_util_test.dir/build.make
src/pstd/tests/pstd_util_test: bin/libpstd.a
src/pstd/tests/pstd_util_test: lib/libgtest.a
src/pstd/tests/pstd_util_test: _deps/spdlog-build/libspdlogd.a
src/pstd/tests/pstd_util_test: _deps/fmt-build/libfmtd.a
src/pstd/tests/pstd_util_test: src/pstd/tests/CMakeFiles/pstd_util_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable pstd_util_test"
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pstd_util_test.dir/link.txt --verbose=$(VERBOSE)
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && /opt/clion/clion-2023.2.2/bin/cmake/linux/x64/bin/cmake -D TEST_TARGET=pstd_util_test -D TEST_EXECUTABLE=/home/pan/MyRepo/pikiwidb/src/pstd/tests/pstd_util_test -D TEST_EXECUTOR= -D TEST_WORKING_DIR=/home/pan/MyRepo/pikiwidb/src/pstd/tests -D TEST_EXTRA_ARGS= -D TEST_PROPERTIES= -D TEST_PREFIX= -D TEST_SUFFIX= -D TEST_FILTER= -D NO_PRETTY_TYPES=FALSE -D NO_PRETTY_VALUES=FALSE -D TEST_LIST=pstd_util_test_TESTS -D CTEST_FILE=/home/pan/MyRepo/pikiwidb/src/pstd/tests/pstd_util_test[1]_tests.cmake -D TEST_DISCOVERY_TIMEOUT=5 -D TEST_XML_OUTPUT_DIR= -P /opt/clion/clion-2023.2.2/bin/cmake/linux/x64/share/cmake-3.26/Modules/GoogleTestAddTests.cmake

# Rule to build all files generated by this target.
src/pstd/tests/CMakeFiles/pstd_util_test.dir/build: src/pstd/tests/pstd_util_test
.PHONY : src/pstd/tests/CMakeFiles/pstd_util_test.dir/build

src/pstd/tests/CMakeFiles/pstd_util_test.dir/clean:
	cd /home/pan/MyRepo/pikiwidb/src/pstd/tests && $(CMAKE_COMMAND) -P CMakeFiles/pstd_util_test.dir/cmake_clean.cmake
.PHONY : src/pstd/tests/CMakeFiles/pstd_util_test.dir/clean

src/pstd/tests/CMakeFiles/pstd_util_test.dir/depend:
	cd /home/pan/MyRepo/pikiwidb && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pan/MyRepo/pikiwidb /home/pan/MyRepo/pikiwidb/src/pstd/tests /home/pan/MyRepo/pikiwidb /home/pan/MyRepo/pikiwidb/src/pstd/tests /home/pan/MyRepo/pikiwidb/src/pstd/tests/CMakeFiles/pstd_util_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/pstd/tests/CMakeFiles/pstd_util_test.dir/depend

