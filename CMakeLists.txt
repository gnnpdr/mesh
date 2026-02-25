cmake_minimum_required(VERSION 3.17)
project(vec3 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vec3
    src/main.cpp
)

target_include_directories(vec3 PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
