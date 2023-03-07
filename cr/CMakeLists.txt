# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (cr)

link_directories($ENV{HOME}/workspace/replication-protos/eRPC/build)
link_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/usr/lib)
include_directories($ENV{HOME}/workspace/replication-protos/eRPC/src)
include_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/x86_64-native-linuxapp-gcc/include)

SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -fsanitize=address")
find_library(FMT_lib fmt)
if(NOT FMT_lib)
    message(STATUS "fmt library not found")
endif()

add_executable(cr main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(cr_donna main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(cr_rose main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(cr_martha main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)



target_link_libraries(cr -lpthread -lfmt -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(cr_donna -lpthread -lfmt -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(cr_rose -lpthread -lfmt -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(cr_martha -lpthread -lfmt -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
