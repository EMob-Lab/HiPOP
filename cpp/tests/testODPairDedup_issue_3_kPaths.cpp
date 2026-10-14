#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <cmath>

/**
 * Consider this graph for example:
 *
 * ```
 *         1           1
 *  A --------> B --------> C
 *  |           |           |
 * 5|           |5          |1
 *  |           |           |
 *  v           v           v
 *  D --------> E --------> F
 *        5           1
 * ```
 *
 * There are 3 possible paths in this graph from A to F:
 * - (P1) ABCF with cost 3
 * - (P2) ABEF with cost 7
 * - (P3) ADEF with cost 11
 *
 * The KShortestPath does not guarantee to find them in order of increasing costs.
 * For example, with cost-multiplier == 10, (P3) is found before (P2).
 *
 * Bug: in `parallelKShortestPath()`, OD pairs that differ from each other only by their respective `kPaths`
 * are computed once, with the largest `kPaths` (see function `find_duplicates()` in cpp/src/shortest_path.cpp).
 * The post-process at the end of `parallelKShortestPath()` then keeps, for each OD pair, the `kPaths` paths
 * with the lowest costs. This assumes that `KShortestPath()` returns the paths in order of increasing costs,
 * which is not the case (see above).
 *
 * Consequence: the paths returned for an OD pair may differ from the ones obtained with a call
 * to `parallelKShortestPath()` (or `KShortestPath()`) with this OD pair alone.
 */
int testODPairDedup_issue_3_kPaths(int, char**)
{

    hipop::OrientedGraph G;
    G.AddNode("A", 0, 0);
    G.AddNode("B", 1, 0);
    G.AddNode("C", 2, 0);
    G.AddNode("D", 0, 1);
    G.AddNode("E", 1, 1);
    G.AddNode("F", 2, 1);
    G.AddLink("A_B", "A", "B", 1, {{"CAR", {{"time", 1}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 1, {{"CAR", {{"time", 1}}}}, "CarLayer");
    G.AddLink("D_E", "D", "E", 1, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.AddLink("E_F", "E", "F", 1, {{"CAR", {{"time", 1}}}}, "CarLayer");
    G.AddLink("A_D", "A", "D", 1, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.AddLink("B_E", "B", "E", 1, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.AddLink("C_F", "C", "F", 1, {{"CAR", {{"time", 1}}}}, "CarLayer");

    std::unordered_map<std::string, std::string> labelToCostFamily = {{"CarLayer", "CAR"}};

    auto expectedPathsWithK2 = hipop::KShortestPath(G, "A", "F", "time", {}, labelToCostFamily, INFINITY, INFINITY, 10, 1, 2, false);
    assertTrue(expectedPathsWithK2.size() == 2, "Wrong number of paths A->F found for K=2");
    assertEqualPaths(expectedPathsWithK2[0], pathCost{{"A", "B", "C", "F"}, 3}, "Wrong path #0 for K=2");
    assertEqualPaths(expectedPathsWithK2[1], pathCost{{"A", "D", "E", "F"}, 11}, "Wrong path #1 for K=2");

    auto expectedPathsWithK3 = hipop::KShortestPath(G, "A", "F", "time", {}, labelToCostFamily, INFINITY, INFINITY, 10, 1, 3, false);
    assertTrue(expectedPathsWithK3.size() == 3, "Wrong number of paths A->F found for K=3");
    assertEqualPaths(expectedPathsWithK3[0], pathCost{{"A", "B", "C", "F"}, 3}, "Wrong path #0 for K=3");
    assertEqualPaths(expectedPathsWithK3[1], pathCost{{"A", "D", "E", "F"}, 11}, "Wrong path #1 for K=3");
    assertEqualPaths(expectedPathsWithK3[2], pathCost{{"A", "B", "E", "F"}, 7}, "Wrong path #2 for K=3");

    // Same OD pairs in a single batch: the results should be identical to the ones obtained with separate calls.
    auto batchedPaths = hipop::parallelKShortestPath(
        G,
        {"A", "A"},
        {"F", "F"},
        "time",
        {labelToCostFamily, labelToCostFamily},
        {},
        INFINITY,
        INFINITY,
        10,
        1,
        {2, 3}, // Identical OD pairs, except here (`kPaths` parameter).
        1
    );
    assertTrue(batchedPaths.size() == 2, "Wrong number of OD pairs in batch");

    assertTrue(batchedPaths[0].size() == 2, "Wrong number of paths A->F found for K=2 in batch");
    assertEqualPaths(batchedPaths[0][0], expectedPathsWithK2[0], "Wrong path #0 for K=2 in batch");
    assertEqualPaths(batchedPaths[0][1], expectedPathsWithK2[1], "Wrong path #1 for K=2 in batch");

    assertTrue(batchedPaths[1].size() == 3, "Wrong number of paths A->F found for K=3 in batch");
    assertEqualPaths(batchedPaths[1][0], expectedPathsWithK3[0], "Wrong path #0 for K=3 in batch");
    assertEqualPaths(batchedPaths[1][1], expectedPathsWithK3[1], "Wrong path #1 for K=3 in batch");
    assertEqualPaths(batchedPaths[1][2], expectedPathsWithK3[2], "Wrong path #2 for K=3 in batch");

    return 0;
}
