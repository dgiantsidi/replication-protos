# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

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
CMAKE_COMMAND = /nix/store/pg9xkxbscvfybwha04s118j9gikws31q-cmake-3.22.3/bin/cmake

# The command to remove a file.
RM = /nix/store/pg9xkxbscvfybwha04s118j9gikws31q-cmake-3.22.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/dimitra/workspace/replication-protos/alc

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/dimitra/workspace/replication-protos/alc/build

# Include any dependencies generated for this target.
include CMakeFiles/acl_rose.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/acl_rose.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/acl_rose.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/acl_rose.dir/flags.make

CMakeFiles/acl_rose.dir/main.cc.o: CMakeFiles/acl_rose.dir/flags.make
CMakeFiles/acl_rose.dir/main.cc.o: ../main.cc
CMakeFiles/acl_rose.dir/main.cc.o: CMakeFiles/acl_rose.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/dimitra/workspace/replication-protos/alc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/acl_rose.dir/main.cc.o"
	/nix/store/gfr4ljdlr1wplc01j4fchw4w9x3lfvv9-gcc-wrapper-11.3.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/acl_rose.dir/main.cc.o -MF CMakeFiles/acl_rose.dir/main.cc.o.d -o CMakeFiles/acl_rose.dir/main.cc.o -c /home/dimitra/workspace/replication-protos/alc/main.cc

CMakeFiles/acl_rose.dir/main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/acl_rose.dir/main.cc.i"
	/nix/store/gfr4ljdlr1wplc01j4fchw4w9x3lfvv9-gcc-wrapper-11.3.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/dimitra/workspace/replication-protos/alc/main.cc > CMakeFiles/acl_rose.dir/main.cc.i

CMakeFiles/acl_rose.dir/main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/acl_rose.dir/main.cc.s"
	/nix/store/gfr4ljdlr1wplc01j4fchw4w9x3lfvv9-gcc-wrapper-11.3.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/dimitra/workspace/replication-protos/alc/main.cc -o CMakeFiles/acl_rose.dir/main.cc.s

# Object files for target acl_rose
acl_rose_OBJECTS = \
"CMakeFiles/acl_rose.dir/main.cc.o"

# External object files for target acl_rose
acl_rose_EXTERNAL_OBJECTS =

acl_rose: CMakeFiles/acl_rose.dir/main.cc.o
acl_rose: CMakeFiles/acl_rose.dir/build.make
acl_rose: CMakeFiles/acl_rose.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/dimitra/workspace/replication-protos/alc/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable acl_rose"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/acl_rose.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/acl_rose.dir/build: acl_rose
.PHONY : CMakeFiles/acl_rose.dir/build

CMakeFiles/acl_rose.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/acl_rose.dir/cmake_clean.cmake
.PHONY : CMakeFiles/acl_rose.dir/clean

CMakeFiles/acl_rose.dir/depend:
	cd /home/dimitra/workspace/replication-protos/alc/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/dimitra/workspace/replication-protos/alc /home/dimitra/workspace/replication-protos/alc /home/dimitra/workspace/replication-protos/alc/build /home/dimitra/workspace/replication-protos/alc/build /home/dimitra/workspace/replication-protos/alc/build/CMakeFiles/acl_rose.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/acl_rose.dir/depend

