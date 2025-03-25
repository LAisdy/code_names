cmake_minimum_required(VERSION 3.15)
project(CodeNames)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Путь к SFML

set(SFML_DIR "${CMAKE_SOURCE_DIR}/libs/SFML/lib/cmake/SFML")

set(SFML_STATIC_LIBRARIES ON)

link_directories("${CMAKE_SOURCE_DIR}/libs/SFML/lib")

find_package(SFML REQUIRED COMPONENTS 
    graphics 
    window 
    system 
    network
)

add_subdirectory(server)
add_subdirectory(client)