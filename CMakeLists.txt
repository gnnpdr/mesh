cmake_minimum_required(VERSION 3.17)
project(mesh CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(POLYSCOPE_BUILD_SHARED_LIBS "Build polyscope as shared libs" OFF)

add_subdirectory(polyscope)

set(SOURCES
    src/main.cpp
)

add_executable(mesh ${SOURCES})

target_link_libraries(mesh 
    polyscope
    ${CMAKE_DL_LIBS}
    glfw
)

target_include_directories(mesh PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(mesh opengl32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(mesh ${COCOA_LIBRARY})
else()
    target_link_libraries(mesh GL)
endif()