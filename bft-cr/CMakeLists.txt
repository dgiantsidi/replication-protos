# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (bft-cr)

link_directories($ENV{HOME}/workspace/replication-protos/eRPC/build)
link_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/usr/lib)
include_directories($ENV{HOME}/workspace/replication-protos/eRPC/src)
include_directories($ENV{HOME}/workspace/replication-protos/sha256-example/digital_signatures)
include_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/x86_64-native-linuxapp-gcc/include)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)
if ( PRINTS_ON )
	SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -fsanitize=address -DDEBUG_PRINT")
else ()
	SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -fsanitize=address")
endif()
find_library(FMT_lib fmt)
if(NOT FMT_lib)
    message(STATUS "fmt library not found")
endif()

add_executable(bftcr main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(bftcr_donna main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(bftcr_rose main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(bftcr_martha main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)



target_link_libraries(bftcr -lpthread -lfmt -lnuma -lerpc -lgflags -lssl -lcrypto -lcityhash -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(bftcr_donna -lpthread -lfmt -lnuma -lerpc -lssl -lcrypto -lcityhash -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(bftcr_rose -lpthread -lfmt -lnuma -lerpc -lssl -lcrypto -lcityhash -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
target_link_libraries(bftcr_martha -lpthread -lfmt -lnuma -lerpc -lssl -lcrypto  -lcityhash -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port -fsanitize=address)
