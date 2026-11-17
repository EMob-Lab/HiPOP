#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <functional>
#include <iostream>
#include <string_view>

using namespace hipop;


static void testCase(
    std::string_view test_case_name,
    const std::function<OrientedGraph()> &graph_factory,
    const std::function<void(const OrientedGraph&)> &test_fun
) {
    std::cout << "TEST CASE " << test_case_name << std::endl;
    OrientedGraph G = graph_factory();
    test_fun(G);
}


int testDijkstraSingleSource(int, char**) {

    testCase("Simple graph (car layer only)", []() { return simple_graph(false); }, [](const OrientedGraph &G) {
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("A_B")},
                {G.node("C"), G.link("B_C")},
                {G.node("D"), G.link("A_D")},
                {G.node("E"), G.link("D_E")},
                {G.node("F"), G.link("C_F")}
            }, "Origin A");
        }
        {
            auto tree = dijkstraSingleSource(G, "B", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("C"), G.link("B_C")},
                {G.node("E"), G.link("B_E")},
                {G.node("F"), G.link("C_F")}
            }, "Origin B");
        }
        {
            auto tree = dijkstraSingleSource(G, "D", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("E"), G.link("D_E")},
                {G.node("F"), G.link("E_F")}
            }, "Origin D");
        }
    });

    testCase("Simple graph (car & bus layers)", []() { return simple_graph(true); }, [](const OrientedGraph &G) {

        // Both layers accessible
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("A_B")},
                {G.node("C"), G.link("B_C")},
                {G.node("D"), G.link("A_D")},
                {G.node("E"), G.link("A_E")},
                {G.node("F"), G.link("E_F")},
                {G.node("I"), G.link("E_I")}
            }, "Origin A");
        }
        {
            auto tree = dijkstraSingleSource(G, "D", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("E"), G.link("D_E")},
                {G.node("F"), G.link("E_F")},
                {G.node("I"), G.link("E_I")}
            }, "Origin D");
        }

        // Only the car layer accessible (equivalent to test case "Simple graph (car layer only)")
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("A_B")},
                {G.node("C"), G.link("B_C")},
                {G.node("D"), G.link("A_D")},
                {G.node("E"), G.link("D_E")},
                {G.node("F"), G.link("C_F")}
            }, "Origin A with car layer only");
        }

        // Only the bus layer accessible
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualShortestPathTrees(tree, {
                {G.node("E"), G.link("A_E")},
                {G.node("I"), G.link("E_I")}
            }, "Origin A with bus layer only");
        }
        {
            auto tree = dijkstraSingleSource(G, "D", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualShortestPathTrees(tree, {
                {G.node("I"), G.link("D_I")}
            }, "Origin D with bus layer only");
        }
    });

    testCase("Special cost values", graph_with_special_cost_values, [](const OrientedGraph &G) {

        // Cases with a zero cost on a link
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("A_B")},
                {G.node("C"), G.link("B_C")},
                {G.node("D"), G.link("C_D")}
            }, "Origin A with time");
        }
        {
            auto tree = dijkstraSingleSource(G, "B", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("C"), G.link("B_C")},
                {G.node("D"), G.link("C_D")}
            }, "Origin B with time");
        }

        // Cases with a +infinity cost on a link
        {
            auto tree = dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("A_B")},
                {G.node("C"), G.link("A_C")},
                {G.node("D"), G.link("B_D")}
            }, "Origin A with generalized cost");
        }
        {
            auto tree = dijkstraSingleSource(G, "B", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                // C not reachable since +inf links are impassable.
                {G.node("D"), G.link("B_D")}
            }, "Origin B with generalized cost");
        }
    });

    testCase("Loops", graph_with_loop, [](const OrientedGraph &G) {

        // Non-zero cycle cost.
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("L1_B")},
                {G.node("C"), G.link("L2_C")},
                {G.node("L1"), G.link("A_L1")},
                {G.node("L2"), G.link("L1_L2")},
                {G.node("X"), G.link("L1_X")},
                {G.node("Y"), G.link("L2_Y")}
            }, "Origin A with non-zero cycle cost");
        }

        // Zero cycle cost.
        {
            auto tree = dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("L1_B")},
                {G.node("C"), G.link("L2_C")},
                {G.node("L1"), G.link("A_L1")},
                {G.node("L2"), G.link("L1_L2")},
                {G.node("X"), G.link("L1_X")},
                {G.node("Y"), G.link("L2_Y")}
            }, "Origin A with zero cycle cost");
        }
    });

    testCase("Self-links", graph_with_self_link, [](const OrientedGraph &G) {

        // Non-zero self-link cost.
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("L_B")},
                {G.node("L"), G.link("A_L")},
                {G.node("X"), G.link("L_X")}
            }, "Origin A with non-zero self-link cost");
        }

        // Zero self-link cost.
        {
            auto tree = dijkstraSingleSource(G, "A", "generalized_cost", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B"), G.link("L_B")},
                {G.node("L"), G.link("A_L")},
                {G.node("X"), G.link("L_X")}
            }, "Origin A with zero self-link cost");
        }
    });

    testCase("Forbidden transitions", graph_with_exclude_movements, [](const OrientedGraph &G) {
        {
            auto tree = dijkstraSingleSource(G, "A", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("B1"), G.link("A_B1")},
                {G.node("B2"), G.link("A_B2")},
                {G.node("C"), G.link("B1_C")}
            }, "Origin A (cannot transit from A to C via B2)");
        }
        {
            auto tree = dijkstraSingleSource(G, "B2", "time", {{"CarLayer", "CAR"}}, {});
            assertEqualShortestPathTrees(tree, {
                {G.node("C"), G.link("B2_C")}
            }, "Origin B2");
        }
    });

    std::cout << "DONE" << std::endl;
    return 0;
}
