cmake_minimum_required(VERSION 3.10)

project(cchat)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE you_werent_supposed_to_be_here)

find_package(raylib)

set(SERVER_SRC
   src/server.c
)

set(CLIENT_SRC
   src/client.c
)

add_executable(cchat-server ${SERVER_SRC})
target_include_directories(cchat-server PRIVATE src/)
target_compile_options(cchat-server PRIVATE -Wall -Wextra -Wpedantic)

add_executable(cchat-client ${CLIENT_SRC})
target_include_directories(cchat-client PRIVATE src/)
target_link_libraries(cchat-client PRIVATE raylib)
target_compile_options(cchat-client PRIVATE -Wall -Wextra -Wpedantic)
