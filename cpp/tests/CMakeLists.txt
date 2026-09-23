set(Tests
    testAStar.cpp
    testCopyGraph.cpp
    testGraph.cpp
    testGraphMerge.cpp
    testDijkstra.cpp
    testDijkstraSingleSource.cpp
    testKShortestPath.cpp
    testYenKShortestPath.cpp
    testManhattan.cpp
    testparallelKShortestPath.cpp
)

# Create a single executable (aka. the driver) capable of running each registered test.
create_test_sourcelist(CommonCxxTestsSrc CommonCxxTests.cpp ${Tests})
add_executable(CommonCxxTests ${CommonCxxTestsSrc})
target_link_libraries(CommonCxxTests hipoplib)

# Register each test with CTest.
foreach(test ${Tests})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${TName} COMMAND CommonCxxTests ${TName})
endforeach()
