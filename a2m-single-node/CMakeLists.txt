# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (a2m-single-node)

SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -O3")
SET(EXTRA_FLAGS "")
if (SANITIZE)
	message(STATUS "fsanitize=address is on")
	SET(CMAKE_CXX_FLAGS  "-DERPC_DPDK=true -g -O3 -fsanitize=address")
	SET(EXTRA_FLAGS "-fsanitize=address")
endif()

link_directories($ENV{HOME}/workspace/replication-protos/eRPC/build)
link_directories(/usr/local/lib64)
link_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/usr/lib)
include_directories($ENV{HOME}/workspace/replication-protos/eRPC/src)
include_directories($ENV{HOME}/workspace/replication-protos/dpdk-stable-19.11.14/x86_64-native-linuxapp-gcc/include)
include_directories($ENV{HOME}/workspace/replication-protos/)
include_directories($ENV{HOME}/workspace/replication-protos/crypto/)

find_library(FMT_lib fmt)
if(NOT FMT_lib)
    message(STATUS "fmt library not found")
endif()

#add_executable(log_donna main.cc $ENV{HOME}/workspace/replication-protos/eRPC/build/src/config.h)
add_executable(test_log test_log.cc)



#target_link_libraries(log_donna -lpthread -lfmt -lcrypto -lnuma -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
#target_link_libraries(log_martha -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
#target_link_libraries(log_amy -lpthread -lfmt -lnuma -lcrypto -lerpc -lgflags -ldl -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lrte_ethdev -Wl,-lrte_port ${EXTRA_FLAGS})
target_link_libraries(test_log  -lcrypto -lfmt ${EXTRA_FLAGS})
