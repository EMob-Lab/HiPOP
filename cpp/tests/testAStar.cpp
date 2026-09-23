#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

#include <algorithm>
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


/**
 * Build a penalty heuristic to use with the A* shortest path algorithm.
 *
 * All the test cases below uses "time" as the cost metric, thus the penality heuristic
 * used here is the straight-line distance between two nodes divided by the maximum speed
 * (i.e. the minimum possible travel time between two nodes).
 */
static std::function<double(const hipop::Node*, const hipop::Node*)> makePenaltyFun(double max_speed) {
    return [max_speed](const hipop::Node *a, const hipop::Node *b) {
        double dx = b->mposition[0] - a->mposition[0];
        double dy = b->mposition[1] - a->mposition[1];
        return std::sqrt(dx * dx + dy * dy) / max_speed;
    };
}


int testAStar(int, char**) {

    testCase("Simple graph (car layer only)", []() { return simple_graph(false); }, [](const hipop::OrientedGraph &G) {

        auto penalty = makePenaltyFun(SIMPLE_GRAPH_CAR_MAX_SPEED);

        // Standard cases
        {
            auto path = hipop::aStar(G, "A", "E", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{"A", "D", "E"}, 8}, "A -> E");
        }
        {
            auto path = hipop::aStar(G, "B", "F", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{"B", "C", "F"}, 5.5}, "B -> F");
        }
        {
            auto path = hipop::aStar(G, "A", "F", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{"A", "B", "C", "F"}, 10.5}, "A -> F");
        }

        // Edge cases
        {
            auto path = hipop::aStar(G, "A", "A", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A");
        }
        {
            auto path = hipop::aStar(G, "B", "B", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path B -> B");
        }
        {
            auto path = hipop::aStar(G, "A", "I", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{}, INFINITY}, "Non-feasible path A -> I");
        }
        {
            auto path = hipop::aStar(G, "D", "A", "time", {{"CarLayer", "CAR"}}, {}, penalty);
            assertEqualPaths(path, pathCost{{}, INFINITY}, "Non-feasible path D -> A");
        }
    });

    testCase("Simple graph (car & bus layers)", []() { return simple_graph(true); }, [](const hipop::OrientedGraph &G) {

        auto car_bus_penalty = makePenaltyFun(std::max(SIMPLE_GRAPH_CAR_MAX_SPEED, SIMPLE_GRAPH_BUS_MAX_SPEED));
        auto car_only_penalty = makePenaltyFun(SIMPLE_GRAPH_CAR_MAX_SPEED);
        auto bus_only_penalty = makePenaltyFun(SIMPLE_GRAPH_BUS_MAX_SPEED);

        // Cases with both layers accessible
        {
            auto path = hipop::aStar(G, "A", "E", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {}, car_bus_penalty);
            assertEqualPaths(path, pathCost{{"A", "E"}, 7}, "A -> E");
        }
        {
            auto path = hipop::aStar(G, "D", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {}, car_bus_penalty);
            assertEqualPaths(path, pathCost{{"D", "E", "I"}, 3}, "D -> I");
        }
        {
            auto path = hipop::aStar(G, "A", "I", "time", {{"CarLayer", "CAR"}, {"BusLayer", "BUS"}}, {}, car_bus_penalty);
            assertEqualPaths(path, pathCost{{"A", "E", "I"}, 8}, "A -> I");
        }

        // Cases with only the car layer accessible (equivalent to test case "Simple graph (car layer only)")
        {
            auto path = hipop::aStar(G, "A", "E", "time", {{"CarLayer", "CAR"}}, {"CarLayer"}, car_only_penalty);
            assertEqualPaths(path, pathCost{{"A", "D", "E"}, 8}, "A -> E with car layer only");
        }
        {
            auto path = hipop::aStar(G, "A", "I", "time", {{"CarLayer", "CAR"}}, {"CarLayer"}, car_only_penalty);
            assertEqualPaths(path, pathCost{{}, INFINITY}, "A -> I with car layer only");
        }
        {
            auto path = hipop::aStar(G, "A", "A", "time", {{"CarLayer", "CAR"}}, {"CarLayer"}, car_only_penalty);
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A with car layer only");
        }

        // Cases with only the bus layer accessible
        {
            auto path = hipop::aStar(G, "A", "E", "time", {{"BusLayer", "BUS"}}, {"BusLayer"}, bus_only_penalty);
            assertEqualPaths(path, pathCost{{"A", "E"}, 7}, "A -> E with bus layer only");
        }
        {
            auto path = hipop::aStar(G, "D", "I", "time", {{"BusLayer", "BUS"}}, {"BusLayer"}, bus_only_penalty);
            assertEqualPaths(path, pathCost{{"D", "I"}, 4}, "D -> I with bus layer only");
        }
        {
            auto path = hipop::aStar(G, "A", "A", "time", {{"BusLayer", "BUS"}}, {"BusLayer"}, bus_only_penalty);
            assertEqualPaths(path, pathCost{{}, 0}, "Empty path A -> A with bus layer only");
        }
    });

    std::cout << "DONE" << std::endl;
    return 0;
}
