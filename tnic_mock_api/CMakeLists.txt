# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (tnic_mock_api)

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

add_executable(test_api test_api.cc)



target_link_libraries(test_api  -lcrypto -lfmt ${EXTRA_FLAGS})
