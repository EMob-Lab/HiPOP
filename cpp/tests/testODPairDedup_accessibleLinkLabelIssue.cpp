#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>


/**
 * Bug: the OD ("origin-destination") pair deduplication mechanism used in `parallelDijkstra()`
 * does not consider the `accessibleLinkLabels`. See function `find_duplicates()` in cpp/src/shortest_path.cpp.
 *
 * Consequences:
 * - If `parallelDijkstra()` is called with OD pairs that differ from each other only
 *   by their respective `accessibleLinkLabels`, some of them get the path computed
 *   for a set of links different from what is requested.
 * - Same issue with all the other functions that use `find_duplicates()`,
 *   especially `parallelKShortestPath()`.
 */
int testODPairDedup_accessibleLinkLabelIssue(int, char**)
{
    hipop::OrientedGraph G = simple_graph(true);

    std::unordered_map<std::string, std::string> labelToCostFamily = {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}};
    setstring carOnly = {"CarLayer"};
    setstring carAndBus = {"CarLayer", "BusLayer"};

    const pathCost expectedCarOnly = {{"A", "D", "E"}, 8};
    const pathCost expectedCarAndBus = {{"A", "E"}, 7};

    auto paths = hipop::parallelDijkstra(
        G,
        {"A", "A"},
        {"E", "E"},
        {labelToCostFamily, labelToCostFamily},
        "time",
        1,
        {carOnly, carAndBus} // Identical OD pairs, except here (`accessibleLinkLabels` parameter).
    );
    assertTrue(paths.size() == 2, "Expected exactly 2 paths");
    assertEqualPaths(paths[0], expectedCarOnly, "Wrong path for car-only");
    assertEqualPaths(paths[1], expectedCarAndBus, "Wrong path for car+bus");

    return 0;
}
