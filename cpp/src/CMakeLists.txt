set(HIPOP_SRC
  create.cpp
  graph.cpp
  shortest_path.cpp
)

add_library(hipoplib ${HIPOP_SRC})
set_target_properties(hipoplib PROPERTIES POSITION_INDEPENDENT_CODE ON)  # Needed to link the hipop static lib to the pybind dynamic one

target_link_libraries(hipoplib PUBLIC OpenMP::OpenMP_CXX)

if(APPLE)
    target_link_directories(hipoplib PRIVATE /opt/homebrew/opt/libomp/lib)
endif()

set_property(TARGET hipoplib PROPERTY CXX_STANDARD 17)

target_include_directories( hipoplib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
    /opt/homebrew/include
)

# Trouver OpenMP
find_package(OpenMP REQUIRED)

# Lier OpenMP
target_link_libraries(hipoplib PUBLIC OpenMP::OpenMP_CXX)

# Cas spécifique macOS : forcer les bons flags pour AppleClang
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  message(STATUS "Configuring hipoplib for macOS with OpenMP")

  # Nécessaire pour compiler avec OpenMP avec AppleClang
  target_compile_options(hipoplib PRIVATE -Xpreprocessor -fopenmp)

  # Lier explicitement la bibliothèque OpenMP
  target_link_libraries(hipoplib PRIVATE omp)

  # Spécifie où chercher libomp.dylib
  target_link_directories(hipoplib PRIVATE /opt/homebrew/opt/libomp/lib)
endif()