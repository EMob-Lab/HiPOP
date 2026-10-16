#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/create.h>
#include <hipop/graph.h>
#include <hipop/graph_path.h>
#include <hipop/graph_path_cost.h>
#include <hipop/shortest_path.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <optional>
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


int testDijkstra(int, char**) {

    testCase("Simple graph (car layer only)", []() { return simple_graph(false); }, [](const OrientedGraph &G) {

        // Standard cases
        {
            auto path = dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_D", "D_E"}), 8}, "A -> E");
        }
        {
            auto path = dijkstra(G, "B", "F", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"B_C", "C_F"}), 5.5}, "B -> F");
        }
        {
            auto path = dijkstra(G, "A", "F", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B", "B_C", "C_F"}), 10.5}, "A -> F");
        }

        // Edge cases
        {
            auto path = dijkstra(G, "A", "A", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makeEmptyPath(G, "A"), 0}, "Empty path A -> A");
        }
        {
            auto path = dijkstra(G, "B", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makeEmptyPath(G, "B"), 0}, "Empty path B -> B");
        }
        {
            auto path = dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, std::nullopt, "Non-feasible path A -> I");
        }
        {
            auto path = dijkstra(G, "D", "A", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, std::nullopt, "Non-feasible path D -> A");
        }
    });

    testCase("Simple graph (car & bus layers)", []() { return simple_graph(true); }, [](const OrientedGraph &G) {

        // Cases with both layers accessible
        {
            auto path = dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_E"}), 7}, "A -> E");
        }
        {
            auto path = dijkstra(G, "D", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, PathCost{makePath(G, {"D_E", "E_I"}), 3}, "D -> I");
        }
        {
            auto path = dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_E", "E_I"}), 8}, "A -> I");
        }

        // Cases with only the car layer accessible (equivalent to test case "Simple graph (car layer only)")
        {
            auto path = dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, PathCost{makePath(G, {"A_D", "D_E"}), 8}, "A -> E with car layer only");
        }
        {
            auto path = dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, std::nullopt, "A -> I with car layer only");
        }
        {
            auto path = dijkstra(G, "A", "A", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, PathCost{makeEmptyPath(G, "A"), 0}, "Empty path A -> A with car layer only");
        }

        // Cases with only the bus layer accessible
        {
            auto path = dijkstra(G, "A", "E", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, PathCost{makePath(G, {"A_E"}), 7}, "A -> E with bus layer only");
        }
        {
            auto path = dijkstra(G, "D", "I", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, PathCost{makePath(G, {"D_I"}), 4}, "D -> I with bus layer only");
        }
        {
            auto path = dijkstra(G, "A", "A", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, PathCost{makeEmptyPath(G, "A"), 0}, "Empty path A -> A with bus layer only");
        }
    });

    testCase("Special cost values", graph_with_special_cost_values, [](const OrientedGraph &G) {

        // Cases with a zero cost on a link
        {
            auto path = dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B", "B_C"}), 1}, "A -> C with time");
        }
        {
            auto path = dijkstra(G, "A", "D", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B", "B_C", "C_D"}), 2}, "A -> D with time");
        }
        {
            auto path = dijkstra(G, "B", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"B_C"}), 0}, "B -> C with time (zero cost but non-empty)");
        }

        // Cases with a +infinity cost on a link
        {
            auto path = dijkstra(G, "A", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_C"}), 2.5}, "A -> C with generalized cost");
        }
        {
            auto path = dijkstra(G, "A", "D", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B", "B_D"}), 3}, "A -> D with generalized cost");
        }
        {
            auto path = dijkstra(G, "B", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, std::nullopt, "B -> C with generalized cost (considered as non-feasible)");
        }
    });

    testCase("Loops", graph_with_loop, [](const OrientedGraph &G) {

        // Non-zero cycle cost.
        {
            auto path = dijkstra(G, "A", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_B"}), 4}, "A -> B with non-zero cycle cost");
        }
        {
            auto path = dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_L2", "L2_C"}), 5}, "A -> C with non-zero cycle cost");
        }

        // Zero cycle cost.
        {
            auto path = dijkstra(G, "A", "B", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_B"}), 4}, "A -> B with zero cycle cost");
        }
        {
            auto path = dijkstra(G, "A", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_L2", "L2_C"}), 4}, "A -> C with zero cycle cost");
        }
        {
            auto path = dijkstra(G, "A", "X", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_X"}), 4}, "A -> X with zero cycle cost");
        }
        {
            auto path = dijkstra(G, "A", "Y", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L1", "L1_L2", "L2_Y"}), 4}, "A -> Y with zero cycle cost");
        }
    });

    testCase("Self-links", graph_with_self_link, [](const OrientedGraph &G) {

        // Non-zero self-link cost.
        {
            auto path = dijkstra(G, "A", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L", "L_B"}), 4}, "A -> B with non-zero self-link cost");
        }

        // Zero self-link cost.
        {
            auto path = dijkstra(G, "A", "B", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L", "L_B"}), 4}, "A -> B with zero self-link cost");
        }
        {
            auto path = dijkstra(G, "A", "X", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_L", "L_X"}), 4}, "A -> X with zero self-link cost");
        }
    });


    testCase("Forbidden transitions", graph_with_exclude_movements, [](const OrientedGraph &G) {
        {
            auto path = dijkstra(G, "A", "B2", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B2"}), 2}, "A -> B2");
        }
        {
            auto path = dijkstra(G, "B2", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"B2_C"}), 1}, "B2 -> C");
        }
        {
            auto path = dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, PathCost{makePath(G, {"A_B1", "B1_C"}), 7}, "A -> C (cannot transit from A to C via B2)");
        }
    });

    std::cout << "DONE" << std::endl;
    return 0;
}
