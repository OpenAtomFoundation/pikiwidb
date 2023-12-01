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
include src/net/CMakeFiles/net.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/net/CMakeFiles/net.dir/compiler_depend.make

# Include the progress variables for this target.
include src/net/CMakeFiles/net.dir/progress.make

# Include the compile flags for this target's objects.
include src/net/CMakeFiles/net.dir/flags.make

src/net/CMakeFiles/net.dir/config_parser.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/config_parser.cc.o: src/net/config_parser.cc
src/net/CMakeFiles/net.dir/config_parser.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/net/CMakeFiles/net.dir/config_parser.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/config_parser.cc.o -MF CMakeFiles/net.dir/config_parser.cc.o.d -o CMakeFiles/net.dir/config_parser.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/config_parser.cc

src/net/CMakeFiles/net.dir/config_parser.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/config_parser.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/config_parser.cc > CMakeFiles/net.dir/config_parser.cc.i

src/net/CMakeFiles/net.dir/config_parser.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/config_parser.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/config_parser.cc -o CMakeFiles/net.dir/config_parser.cc.s

src/net/CMakeFiles/net.dir/event_loop.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/event_loop.cc.o: src/net/event_loop.cc
src/net/CMakeFiles/net.dir/event_loop.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/net/CMakeFiles/net.dir/event_loop.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/event_loop.cc.o -MF CMakeFiles/net.dir/event_loop.cc.o.d -o CMakeFiles/net.dir/event_loop.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/event_loop.cc

src/net/CMakeFiles/net.dir/event_loop.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/event_loop.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/event_loop.cc > CMakeFiles/net.dir/event_loop.cc.i

src/net/CMakeFiles/net.dir/event_loop.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/event_loop.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/event_loop.cc -o CMakeFiles/net.dir/event_loop.cc.s

src/net/CMakeFiles/net.dir/http_client.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/http_client.cc.o: src/net/http_client.cc
src/net/CMakeFiles/net.dir/http_client.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/net/CMakeFiles/net.dir/http_client.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/http_client.cc.o -MF CMakeFiles/net.dir/http_client.cc.o.d -o CMakeFiles/net.dir/http_client.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/http_client.cc

src/net/CMakeFiles/net.dir/http_client.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/http_client.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/http_client.cc > CMakeFiles/net.dir/http_client.cc.i

src/net/CMakeFiles/net.dir/http_client.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/http_client.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/http_client.cc -o CMakeFiles/net.dir/http_client.cc.s

src/net/CMakeFiles/net.dir/http_parser.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/http_parser.cc.o: src/net/http_parser.cc
src/net/CMakeFiles/net.dir/http_parser.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/net/CMakeFiles/net.dir/http_parser.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/http_parser.cc.o -MF CMakeFiles/net.dir/http_parser.cc.o.d -o CMakeFiles/net.dir/http_parser.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/http_parser.cc

src/net/CMakeFiles/net.dir/http_parser.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/http_parser.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/http_parser.cc > CMakeFiles/net.dir/http_parser.cc.i

src/net/CMakeFiles/net.dir/http_parser.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/http_parser.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/http_parser.cc -o CMakeFiles/net.dir/http_parser.cc.s

src/net/CMakeFiles/net.dir/http_server.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/http_server.cc.o: src/net/http_server.cc
src/net/CMakeFiles/net.dir/http_server.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/net/CMakeFiles/net.dir/http_server.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/http_server.cc.o -MF CMakeFiles/net.dir/http_server.cc.o.d -o CMakeFiles/net.dir/http_server.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/http_server.cc

src/net/CMakeFiles/net.dir/http_server.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/http_server.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/http_server.cc > CMakeFiles/net.dir/http_server.cc.i

src/net/CMakeFiles/net.dir/http_server.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/http_server.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/http_server.cc -o CMakeFiles/net.dir/http_server.cc.s

src/net/CMakeFiles/net.dir/libevent_reactor.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/libevent_reactor.cc.o: src/net/libevent_reactor.cc
src/net/CMakeFiles/net.dir/libevent_reactor.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/net/CMakeFiles/net.dir/libevent_reactor.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/libevent_reactor.cc.o -MF CMakeFiles/net.dir/libevent_reactor.cc.o.d -o CMakeFiles/net.dir/libevent_reactor.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/libevent_reactor.cc

src/net/CMakeFiles/net.dir/libevent_reactor.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/libevent_reactor.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/libevent_reactor.cc > CMakeFiles/net.dir/libevent_reactor.cc.i

src/net/CMakeFiles/net.dir/libevent_reactor.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/libevent_reactor.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/libevent_reactor.cc -o CMakeFiles/net.dir/libevent_reactor.cc.s

src/net/CMakeFiles/net.dir/pipe_obj.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/pipe_obj.cc.o: src/net/pipe_obj.cc
src/net/CMakeFiles/net.dir/pipe_obj.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object src/net/CMakeFiles/net.dir/pipe_obj.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/pipe_obj.cc.o -MF CMakeFiles/net.dir/pipe_obj.cc.o.d -o CMakeFiles/net.dir/pipe_obj.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/pipe_obj.cc

src/net/CMakeFiles/net.dir/pipe_obj.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/pipe_obj.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/pipe_obj.cc > CMakeFiles/net.dir/pipe_obj.cc.i

src/net/CMakeFiles/net.dir/pipe_obj.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/pipe_obj.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/pipe_obj.cc -o CMakeFiles/net.dir/pipe_obj.cc.s

src/net/CMakeFiles/net.dir/tcp_connection.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/tcp_connection.cc.o: src/net/tcp_connection.cc
src/net/CMakeFiles/net.dir/tcp_connection.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object src/net/CMakeFiles/net.dir/tcp_connection.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/tcp_connection.cc.o -MF CMakeFiles/net.dir/tcp_connection.cc.o.d -o CMakeFiles/net.dir/tcp_connection.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/tcp_connection.cc

src/net/CMakeFiles/net.dir/tcp_connection.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/tcp_connection.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/tcp_connection.cc > CMakeFiles/net.dir/tcp_connection.cc.i

src/net/CMakeFiles/net.dir/tcp_connection.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/tcp_connection.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/tcp_connection.cc -o CMakeFiles/net.dir/tcp_connection.cc.s

src/net/CMakeFiles/net.dir/tcp_listener.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/tcp_listener.cc.o: src/net/tcp_listener.cc
src/net/CMakeFiles/net.dir/tcp_listener.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object src/net/CMakeFiles/net.dir/tcp_listener.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/tcp_listener.cc.o -MF CMakeFiles/net.dir/tcp_listener.cc.o.d -o CMakeFiles/net.dir/tcp_listener.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/tcp_listener.cc

src/net/CMakeFiles/net.dir/tcp_listener.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/tcp_listener.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/tcp_listener.cc > CMakeFiles/net.dir/tcp_listener.cc.i

src/net/CMakeFiles/net.dir/tcp_listener.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/tcp_listener.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/tcp_listener.cc -o CMakeFiles/net.dir/tcp_listener.cc.s

src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o: src/net/unbounded_buffer.cc
src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o -MF CMakeFiles/net.dir/unbounded_buffer.cc.o.d -o CMakeFiles/net.dir/unbounded_buffer.cc.o -c /home/pan/MyRepo/pikiwidb/src/net/unbounded_buffer.cc

src/net/CMakeFiles/net.dir/unbounded_buffer.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/net.dir/unbounded_buffer.cc.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/unbounded_buffer.cc > CMakeFiles/net.dir/unbounded_buffer.cc.i

src/net/CMakeFiles/net.dir/unbounded_buffer.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/net.dir/unbounded_buffer.cc.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/unbounded_buffer.cc -o CMakeFiles/net.dir/unbounded_buffer.cc.s

src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o: src/net/lzf/lzf_c.c
src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o -MF CMakeFiles/net.dir/lzf/lzf_c.c.o.d -o CMakeFiles/net.dir/lzf/lzf_c.c.o -c /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_c.c

src/net/CMakeFiles/net.dir/lzf/lzf_c.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/net.dir/lzf/lzf_c.c.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_c.c > CMakeFiles/net.dir/lzf/lzf_c.c.i

src/net/CMakeFiles/net.dir/lzf/lzf_c.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/net.dir/lzf/lzf_c.c.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_c.c -o CMakeFiles/net.dir/lzf/lzf_c.c.s

src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o: src/net/CMakeFiles/net.dir/flags.make
src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o: src/net/lzf/lzf_d.c
src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o: src/net/CMakeFiles/net.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o -MF CMakeFiles/net.dir/lzf/lzf_d.c.o.d -o CMakeFiles/net.dir/lzf/lzf_d.c.o -c /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_d.c

src/net/CMakeFiles/net.dir/lzf/lzf_d.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/net.dir/lzf/lzf_d.c.i"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_d.c > CMakeFiles/net.dir/lzf/lzf_d.c.i

src/net/CMakeFiles/net.dir/lzf/lzf_d.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/net.dir/lzf/lzf_d.c.s"
	cd /home/pan/MyRepo/pikiwidb/src/net && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pan/MyRepo/pikiwidb/src/net/lzf/lzf_d.c -o CMakeFiles/net.dir/lzf/lzf_d.c.s

# Object files for target net
net_OBJECTS = \
"CMakeFiles/net.dir/config_parser.cc.o" \
"CMakeFiles/net.dir/event_loop.cc.o" \
"CMakeFiles/net.dir/http_client.cc.o" \
"CMakeFiles/net.dir/http_parser.cc.o" \
"CMakeFiles/net.dir/http_server.cc.o" \
"CMakeFiles/net.dir/libevent_reactor.cc.o" \
"CMakeFiles/net.dir/pipe_obj.cc.o" \
"CMakeFiles/net.dir/tcp_connection.cc.o" \
"CMakeFiles/net.dir/tcp_listener.cc.o" \
"CMakeFiles/net.dir/unbounded_buffer.cc.o" \
"CMakeFiles/net.dir/lzf/lzf_c.c.o" \
"CMakeFiles/net.dir/lzf/lzf_d.c.o"

# External object files for target net
net_EXTERNAL_OBJECTS =

bin/libnet.a: src/net/CMakeFiles/net.dir/config_parser.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/event_loop.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/http_client.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/http_parser.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/http_server.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/libevent_reactor.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/pipe_obj.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/tcp_connection.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/tcp_listener.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/unbounded_buffer.cc.o
bin/libnet.a: src/net/CMakeFiles/net.dir/lzf/lzf_c.c.o
bin/libnet.a: src/net/CMakeFiles/net.dir/lzf/lzf_d.c.o
bin/libnet.a: src/net/CMakeFiles/net.dir/build.make
bin/libnet.a: src/net/CMakeFiles/net.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pan/MyRepo/pikiwidb/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Linking CXX static library ../../bin/libnet.a"
	cd /home/pan/MyRepo/pikiwidb/src/net && $(CMAKE_COMMAND) -P CMakeFiles/net.dir/cmake_clean_target.cmake
	cd /home/pan/MyRepo/pikiwidb/src/net && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/net.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/net/CMakeFiles/net.dir/build: bin/libnet.a
.PHONY : src/net/CMakeFiles/net.dir/build

src/net/CMakeFiles/net.dir/clean:
	cd /home/pan/MyRepo/pikiwidb/src/net && $(CMAKE_COMMAND) -P CMakeFiles/net.dir/cmake_clean.cmake
.PHONY : src/net/CMakeFiles/net.dir/clean

src/net/CMakeFiles/net.dir/depend:
	cd /home/pan/MyRepo/pikiwidb && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pan/MyRepo/pikiwidb /home/pan/MyRepo/pikiwidb/src/net /home/pan/MyRepo/pikiwidb /home/pan/MyRepo/pikiwidb/src/net /home/pan/MyRepo/pikiwidb/src/net/CMakeFiles/net.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/net/CMakeFiles/net.dir/depend

