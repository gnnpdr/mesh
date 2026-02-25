cmake_minimum_required(VERSION 3.17)
project(mesh CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mesh
    src/main.cpp
)

target_include_directories(mesh PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)