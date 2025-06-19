cmake_minimum_required(VERSION 3.4...3.18)
project(pyhipop)

if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    include_directories(/opt/homebrew/opt/libomp/include)
    link_directories(/opt/homebrew/opt/libomp/lib)
endif()

find_package(pybind11 REQUIRED)
find_package(OpenMP REQUIRED)

pybind11_add_module(cpp 
    hipop/cpp/main.cpp 
    hipop/cpp/graph.cpp
    hipop/cpp/shortest_path.cpp)

set_target_properties(cpp PROPERTIES CXX_STANDARD 17)

target_link_libraries(cpp PRIVATE OpenMP::OpenMP_CXX)

find_package(Hipop REQUIRED)
target_link_libraries(cpp PUBLIC hipoplib)
