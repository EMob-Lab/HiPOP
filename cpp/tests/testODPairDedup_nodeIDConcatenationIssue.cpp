#include "helpers.h"

#include <hipop/create.h>
#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <memory>


/**
 * Bug: the OD ("origin-destination") pair deduplication mechanism used in `parallelDijkstra()`
 * identifies the OD pairs by concatenating the IDs of their origin and destination nodes.
 * See function `find_duplicates()` in cpp/src/shortest_path.cpp.
 *
 * Consequences:
 * - If `parallelDijkstra()` is called with OD pairs whose concatenated node IDs are identical
 *   (e.g. "1"->"23" and "12"->"3"), some of them get the path computed for another OD pair.
 * - Same issue with all the other functions that use `find_duplicates()`,
 *   especially `parallelKShortestPath()`.
 */
int testODPairDedup_nodeIDConcatenationIssue(int, char**)
{
    std::unique_ptr<hipop::OrientedGraph> G(hipop::makeManhattan(12, 1));

    std::unordered_map<std::string, std::string> labelToCostFamily = {{"", "PersonalCar"}};

    auto paths = hipop::parallelDijkstra(
        *G,
        {"1", "12"},
        {"23", "3"}, // Different OD pairs, but "1" + "23" == "12" + "3".
        {labelToCostFamily, labelToCostFamily},
        "length",
        1
    );
    assertTrue(paths.size() == 2, "Expected exactly 2 paths");
    assertTrue(!paths[0].first.empty() && !paths[1].first.empty(), "Expected non-empty paths");
    assertTrue(paths[0].first.front() == "1" && paths[0].first.back() == "23", "Wrong path for 1->23");
    assertTrue(paths[1].first.front() == "12" && paths[1].first.back() == "3", "Wrong path for 12->3");

    return 0;
}
