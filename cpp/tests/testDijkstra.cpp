#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <cmath>
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


int testDijkstra(int, char**) {

    testCase("Simple graph (car layer only)", []() { return simple_graph(false); }, [](const hipop::OrientedGraph &G) {

        // Standard cases
        {
            auto path = hipop::dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "D", "E"}, 8}, "A -> E");
        }
        {
            auto path = hipop::dijkstra(G, "B", "F", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"B", "C", "F"}, 5.5}, "B -> F");
        }
        {
            auto path = hipop::dijkstra(G, "A", "F", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B", "C", "F"}, 10.5}, "A -> F");
        }

        // Edge cases
        {
            auto path = hipop::dijkstra(G, "A", "A", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A");
        }
        {
            auto path = hipop::dijkstra(G, "B", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path B -> B");
        }
        {
            auto path = hipop::dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{}, INFINITY}, "Non-feasible path A -> I");
        }
        {
            auto path = hipop::dijkstra(G, "D", "A", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{}, INFINITY}, "Non-feasible path D -> A");
        }
    });

    testCase("Simple graph (car & bus layers)", []() { return simple_graph(true); }, [](const hipop::OrientedGraph &G) {

        // Cases with both layers accessible
        {
            auto path = hipop::dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, pathCost{{"A", "E"}, 7}, "A -> E");
        }
        {
            auto path = hipop::dijkstra(G, "D", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, pathCost{{"D", "E", "I"}, 3}, "D -> I");
        }
        {
            auto path = hipop::dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}});
            assertEqualPaths(path, pathCost{{"A", "E", "I"}, 8}, "A -> I");
        }

        // Cases with only the car layer accessible (equivalent to test case "Simple graph (car layer only)")
        {
            auto path = hipop::dijkstra(G, "A", "E", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, pathCost{{"A", "D", "E"}, 8}, "A -> E with car layer only");
        }
        {
            auto path = hipop::dijkstra(G, "A", "I", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, pathCost{{}, INFINITY}, "A -> I with car layer only");
        }
        {
            auto path = hipop::dijkstra(G, "A", "A", "time", {{"CarLayer", "CAR"}}, {"CarLayer"});
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A with car layer only");
        }

        // Cases with only the bus layer accessible
        {
            auto path = hipop::dijkstra(G, "A", "E", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, pathCost{{"A", "E"}, 7}, "A -> E with bus layer only");
        }
        {
            auto path = hipop::dijkstra(G, "D", "I", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, pathCost{{"D", "I"}, 4}, "D -> I with bus layer only");
        }
        {
            auto path = hipop::dijkstra(G, "A", "A", "time", {{"BusLayer", "BUS"}}, {"BusLayer"});
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A with bus layer only");
        }
    });

    testCase("Special cost values", graph_with_special_cost_values, [](const hipop::OrientedGraph &G) {

        // Cases with a zero cost on a link
        {
            auto path = hipop::dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B", "C"}, 1}, "A -> C with time");
        }
        {
            auto path = hipop::dijkstra(G, "A", "D", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B", "C", "D"}, 2}, "A -> D with time");
        }
        {
            auto path = hipop::dijkstra(G, "B", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"B", "C"}, 0}, "B -> C with time (zero cost but non-empty)");
        }

        // Cases with a +infinity cost on a link
        {
            auto path = hipop::dijkstra(G, "A", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "C"}, 2.5}, "A -> C with generalized cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "D", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B", "D"}, 3}, "A -> D with generalized cost");
        }
        {
            auto path = hipop::dijkstra(G, "B", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{}, INFINITY}, "B -> C with generalized cost (considered as non-feasible)");
        }
    });

    testCase("Loops", graph_with_loop, [](const hipop::OrientedGraph &G) {

        // Non-zero cycle cost.
        {
            auto path = hipop::dijkstra(G, "A", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "B"}, 4}, "A -> B with non-zero cycle cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "L2", "C"}, 5}, "A -> C with non-zero cycle cost");
        }

        // Zero cycle cost.
        {
            auto path = hipop::dijkstra(G, "A", "B", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "B"}, 4}, "A -> B with zero cycle cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "C", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "L2", "C"}, 4}, "A -> C with zero cycle cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "X", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "X"}, 4}, "A -> X with zero cycle cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "Y", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L1", "L2", "Y"}, 4}, "A -> Y with zero cycle cost");
        }
    });

    testCase("Self-links", graph_with_self_link, [](const hipop::OrientedGraph &G) {

        // Non-zero self-link cost.
        {
            auto path = hipop::dijkstra(G, "A", "B", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L", "B"}, 4}, "A -> B with non-zero self-link cost");
        }

        // Zero self-link cost.
        {
            auto path = hipop::dijkstra(G, "A", "B", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L", "B"}, 4}, "A -> B with zero self-link cost");
        }
        {
            auto path = hipop::dijkstra(G, "A", "X", "generalized_cost", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "L", "X"}, 4}, "A -> X with zero self-link cost");
        }
    });


    testCase("Forbidden transitions", graph_with_exclude_movements, [](const hipop::OrientedGraph &G) {
        {
            auto path = hipop::dijkstra(G, "A", "B2", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B2"}, 2}, "A -> B2");
        }
        {
            auto path = hipop::dijkstra(G, "B2", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"B2", "C"}, 1}, "B2 -> C");
        }
        {
            auto path = hipop::dijkstra(G, "A", "C", "time", {{"CarLayer", "CAR"}});
            assertEqualPaths(path, pathCost{{"A", "B1", "C"}, 7}, "A -> C (cannot transit from A to C via B2)");
        }
    });

    std::cout << "DONE" << std::endl;
    return 0;
}
