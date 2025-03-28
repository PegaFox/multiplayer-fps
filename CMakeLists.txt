cmake_minimum_required(VERSION 3.30)

project(multiplayer-fps)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(SFML)

find_package(SFML 3.0 REQUIRED Graphics Network)

add_executable(client client.cpp)

add_executable(server server.cpp)

target_link_libraries(client SFML::Graphics SFML::Network)

target_link_libraries(server SFML::Graphics SFML::Network)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
