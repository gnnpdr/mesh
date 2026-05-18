cmake_minimum_required(VERSION 3.17)
project(mesh CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(POLYSCOPE_BUILD_SHARED_LIBS "Build polyscope as shared libs" OFF)

add_subdirectory(polyscope)
add_subdirectory(eigen3)

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)



set(SOURCES
    src/main.cpp
    src/mesh.cpp
    src/mesh_viewer.cpp
    src/ray_tracer.cpp
    src/register_algorithms.cpp
    src/vertex_cluster.cpp
    src/edge_collapse.cpp
    src/new.cpp
)

set(HEADERS
    include/mesh/mesh.hpp
    include/mesh/mesh_obj_converter.hpp
    include/mesh/obj_parser.hpp
    include/viewer/mesh_viewer.hpp
    include/viewer/ray_tracer.hpp
    include/algorithms/vertex_cluster.hpp
    include/algorithms/edge_collapse.hpp
    include/algorithms/metrics.hpp
    include/algorithms/adapters.hpp
    include/algorithms/simplifier_interface.hpp
    include/algorithms/simplifier_registry.hpp
    include/algorithms/register_algorithms.hpp
    include/algorithms/new.hpp
)

add_executable(mesh ${SOURCES} ${HEADERS})

target_include_directories(mesh PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/polyscope/include
    ${CMAKE_CURRENT_SOURCE_DIR}/eigen3
)

target_link_libraries(mesh
    polyscope
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    OpenGL::GL
    OpenMP::OpenMP_CXX
)

if(WIN32)
    target_link_libraries(mesh opengl32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(mesh ${COCOA_LIBRARY})
else()
    target_link_libraries(mesh GL)
endif()

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/ DESTINATION ${CMAKE_BINARY_DIR}/lib)