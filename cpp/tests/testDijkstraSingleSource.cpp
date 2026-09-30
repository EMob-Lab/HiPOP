#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <functional>
#include <iostream>
#include <string_view>


static void testCase(
    std::string_view test_case_name,
    const std::function<hipop::OrientedGraph()> &graph_factory,
    const std::function<void(const hipop::OrientedGraph&)> &test_fun
) {
    std::cout << "TEST CASE " << test_case_name << std::endl;
    hipop::OrientedGraph G = graph_factory();
    test_fun(G);
}


int testDijkstraSingleSource(int, char**) {

    testCase("Simple graph (car layer only)", []() { return simple_graph(false); }, [](const hipop::OrientedGraph &G) {
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "A"}, {"C", "B"}, {"D", "A"}, {"E", "D"}, {"F", "C"}, {"I", ""}
            }, "Origin A");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "B", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", "B"}, {"D", ""}, {"E", "B"}, {"F", "C"}, {"I", ""}
            }, "Origin B");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "D", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", ""}, {"D", ""}, {"E", "D"}, {"F", "E"}, {"I", ""}
            }, "Origin D");
        }
    });

    testCase("Simple graph (car & bus layers)", []() { return simple_graph(true); }, [](const hipop::OrientedGraph &G) {

        // Both layers accessible
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "A"}, {"C", "B"}, {"D", "A"}, {"E", "A"}, {"F", "E"}, {"I", "E"}
            }, "Origin A");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "D", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", ""}, {"D", ""}, {"E", "D"}, {"F", "E"}, {"I", "E"}
            }, "Origin D");
        }

        // Only the car layer accessible (equivalent to test case "Simple graph (car layer only)")
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "A"}, {"C", "B"}, {"D", "A"}, {"E", "D"}, {"F", "C"}, {"I", ""}
            }, "Origin A with car layer only");
        }

        // Only the bus layer accessible
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", ""}, {"D", ""}, {"E", "A"}, {"F", ""}, {"I", "E"}
            }, "Origin A with bus layer only");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "D", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", ""}, {"D", ""}, {"E", ""}, {"F", ""}, {"I", "D"}
            }, "Origin D with bus layer only");
        }
    });

    testCase("Special cost values", graph_with_special_cost_values, [](const hipop::OrientedGraph &G) {

        // Cases with a zero cost on a link
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "A"}, {"C", "B"}, {"D", "C"}
            }, "Origin A with time");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "B", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", "B"}, {"D", "C"}
            }, "Origin B with time");
        }

        // Cases with a +infinity cost on a link
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "A"}, {"C", "A"}, {"D", "B"}
            }, "Origin A with generalized cost");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "B", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", ""}, {"C", ""} /* C not reachable since +inf links are impassable */, {"D", "B"}
            }, "Origin B with generalized cost");
        }
    });

    testCase("Loops", graph_with_loop, [](const hipop::OrientedGraph &G) {

        // Non-zero cycle cost.
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "L1"}, {"C", "L2"}, {"L1", "A"}, {"L2", "L1"}, {"X", "L1"}, {"Y", "L2"}
            }, "Origin A with non-zero cycle cost");
        }

        // Zero cycle cost.
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "L1"}, {"C", "L2"}, {"L1", "A"}, {"L2", "L1"}, {"X", "L1"}, {"Y", "L2"}
            }, "Origin A with zero cycle cost");
        }
    });

    testCase("Self-links", graph_with_self_link, [](const hipop::OrientedGraph &G) {

        // Non-zero self-link cost.
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "L"}, {"L", "A"}, {"X", "L"}
            }, "Origin A with non-zero self-link cost");
        }

        // Zero self-link cost.
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B", "L"}, {"L", "A"}, {"X", "L"}
            }, "Origin A with zero self-link cost");
        }
    });

    testCase("Forbidden transitions", graph_with_exclude_movements, [](const hipop::OrientedGraph &G) {
        {
            auto tree = hipop::dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B1", "A"}, {"B2", "A"}, {"C", "B1"}
            }, "Origin A (cannot transit from A to C via B2)");
        }
        {
            auto tree = hipop::dijkstraSingleSource(G, "B2", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathsTrees(tree, ShortestPathsTree{
                {"A", ""}, {"B1", ""}, {"B2", ""}, {"C", "B2"}
            }, "Origin B2");
        }
    });

    std::cout << "DONE" << std::endl;
    return 0;
}
