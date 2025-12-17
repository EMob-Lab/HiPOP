# create the testing file and list of tests
create_test_sourcelist (Tests
  CommonCxxTests.cpp
  testCopyGraph.cpp
  testGraph.cpp
  testGraphMerge.cpp
  testDijkstra1.cpp
  testDijkstra2.cpp
  testKShortestPath.cpp
  testYenKShortestPath.cpp
  testManhattan.cpp
  testparallelKShortestPath.cpp
)

# add the executable
add_executable (CommonCxxTests ${Tests})
target_link_libraries(CommonCxxTests hipoplib)
target_include_directories(CommonCxxTests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET CommonCxxTests PROPERTY CXX_STANDARD 17)

# remove the test driver source file
set (TestsToRun ${Tests})
remove (TestsToRun CommonCxxTests.cpp)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
  get_filename_component (TName ${test} NAME_WE)
  add_test (NAME ${TName} COMMAND CommonCxxTests ${TName})
endforeach ()

# Lier OpenMP
#target_link_libraries(CommonCxxTests PUBLIC OpenMP::OpenMP_CXX)

# Cas spécifique macOS : forcer les bons flags pour AppleClang
if(APPLE AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
  message(STATUS "Configuring CommonCxxTests for macOS with OpenMP")

  # Nécessaire pour compiler avec OpenMP avec AppleClang
  target_compile_options(CommonCxxTests PRIVATE -Xpreprocessor -fopenmp)

  # Lier explicitement la bibliothèque OpenMP
  #target_link_libraries(CommonCxxTests PRIVATE omp)

  # Spécifie où chercher libomp.dylib
  target_link_directories(CommonCxxTests PRIVATE /opt/homebrew/opt/libomp/lib)
endif()