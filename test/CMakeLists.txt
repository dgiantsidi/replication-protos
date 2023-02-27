# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (test)

SET(CMAKE_CXX_FLAGS  "-DERPC_INFINIBAND=true")
find_library(FMT_lib fmt)
if(NOT FMT_lib)
    message(STATUS "fmt library not found")
endif()

add_executable(cr main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)

target_include_directories(cr PUBLIC $ENV{HOME}/workspace/replication-protos/eRPC/third_party/asio/include)
target_include_directories(cr PUBLIC $ENV{HOME}/workspace/replication-protos/eRPC/build)
target_link_directories(cr PUBLIC $ENV{HOME}/workspace/replication-protos/eRPC/build)
# target_include_directories(cr PUBLIC $ENV{HOME}/workspace/replication-protos/eRPC/build/src)
target_include_directories(cr PUBLIC $ENV{HOME}/workspace/replication-protos/eRPC/src)

target_link_libraries(cr -lpthread)
target_link_libraries(cr -lfmt -lnuma -lgflags)
target_link_libraries(cr -l:liberpc.a)
