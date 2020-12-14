cmake_minimum_required(VERSION 3.11)
project(proxies)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")

find_package(OpenSSL 1.1.0 REQUIRED)
find_package(Boost 1.66.0 REQUIRED COMPONENTS system filesystem thread)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
message("${OPENSSL_INCLUDE_DIR}")
message("${OPENSSL_LIBRARIES}")
message("${Boost_INCLUDE_DIR}")
message("${Boost_LIBRARIES}")

include_directories(src/core)
file(GLOB_RECURSE SOURCE_FILES src/core/*.cpp)
message("${SOURCE_FILES}")
message("${SOURCE_COMMON_FILES}")
add_executable(proxies src/main.cpp ${SOURCE_FILES} ${SOURCE_COMMON_FILES})
target_link_libraries(proxies ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})


include(GNUInstallDirs)
install(TARGETS proxies DESTINATION bin)
option(OPENWRT "Build for Openwrt support" OFF)

if (OPENWRT)
    install(DIRECTORY confs/openwrt/ DESTINATION etc/proxies)
elseif (CMAKE_HOST_APPLE)
    install(DIRECTORY confs/darwin/ DESTINATION etc/proxies)
elseif (CMAKE_HOST_UNIX)
    install(DIRECTORY confs/linux/ DESTINATION etc/proxies)
endif ()
