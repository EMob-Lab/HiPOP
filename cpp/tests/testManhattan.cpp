#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/create.h>

#include <cstddef>
#include <iostream>

using namespace hipop;


static void doTestManhattan(int n) {
    std::cout << "TEST CASE with N = " << n << std::endl;

    OrientedGraph G = makeManhattan(n, 10);

    // There are N * N nodes on the Manhattan grid itself, + N side nodes on north/south/west/east of the grid.
    assertTrue(
        G.nodeCount() == static_cast<std::size_t>(n * (n + 4)),
        "Manhattan graph does not have the expected number of nodes"
    );

    // There is (N + 1) * 2 horizontal links on each columns (the * 2 factor accounting for the fact that
    // there are 2 links between each adjacent pairs of nodes - one in each direction).
    // And also (N + 1) * 2 vertical links, for the same reasons.
    assertTrue(
        G.linkCount() == static_cast<std::size_t>(n * (n + 1) * 4),
        "Manhattan graph does not have the expected number of links"
    );
}


int testManhattan(int, char**) {
    doTestManhattan(5);
    doTestManhattan(17);
    std::cout << "DONE" << std::endl;
    return 0;
}
