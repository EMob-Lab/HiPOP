cmake_minimum_required(VERSION 3.5)

project(HipopExample)

find_package(OpenMP REQUIRED)
find_package(Hipop REQUIRED)

include_directories("../include")
link_directories("../build/src")

set( SRCS ParallelKShortestPath.cpp)

add_executable(HipopExample ${SRCS})

set_property(TARGET HipopExample PROPERTY CXX_STANDARD 17)

target_link_libraries(HipopExample hipoplib)

install(TARGETS HipopExample RUNTIME DESTINATION bin)