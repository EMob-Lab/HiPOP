#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>


int testDijkstra3(int, char**)
{
    hipop::OrientedGraph G;

    // ```
    //                    5
    //            D1 + -------> + E
    //               ^          ^
    //               |          |
    //             5 |          | 99
    //               | C        |
    // B2 + -------> + -------> + D2
    //    ^    3     ^    99
    //    |          |
    //    | 3        | 8
    //    |          |
    //  A + -------> + B1
    //         8
    // ```
    //
    // On central node C, transitions are restricted as follows:
    // - From B2, cannot turn to D1,
    // - From B1, cannot turn to D2.
    // In other words, on node C, one must go straight: from B2 to D2, or from B1 to D1.

    G.AddNode("A", 0, 0);
    G.AddNode("B1", 1, 0);
    G.AddNode("B2", 0, 1);
    G.AddNode("C", 1, 1, "", {{"B2", {"D1"}}, {"B1", {"D2"}}});
    G.AddNode("D1", 1, 2);
    G.AddNode("D2", 2, 1);
    G.AddNode("E", 2, 2);

    G.AddLink("A_B1", "A", "B1", 1, {{"PersonalVehicle", {{"time", 8}}}}, "CAR");
    G.AddLink("A_B2", "A", "B2", 1, {{"PersonalVehicle", {{"time", 3}}}}, "CAR");
    G.AddLink("B1_C", "B1", "C", 1, {{"PersonalVehicle", {{"time", 8}}}}, "CAR");
    G.AddLink("B2_C", "B2", "C", 1, {{"PersonalVehicle", {{"time", 3}}}}, "CAR");
    G.AddLink("C_D1", "C", "D1", 1, {{"PersonalVehicle", {{"time", 5}}}}, "CAR");
    G.AddLink("C_D2", "C", "D2", 1, {{"PersonalVehicle", {{"time", 99}}}}, "CAR");
    G.AddLink("D1_E", "D1", "E", 1, {{"PersonalVehicle", {{"time", 5}}}}, "CAR");
    G.AddLink("D2_E", "D2", "E", 1, {{"PersonalVehicle", {{"time", 99}}}}, "CAR");

    auto path = hipop::dijkstra(G, "A", "E", "time", {{"CAR", "PersonalVehicle"}});

    std::cerr << "Path cost: " << path.second << std::endl;
    std::cerr << "Path nodes: ";
    for (const auto& node : path.first) {
        std::cerr << node << " ";
    }
    std::cerr << std::endl;

    assertTrue(path.second==26, "Wrong path cost");
    assertTrue(path.first==std::vector<std::string>{"A", "B1", "C", "D1", "E"}, "Wrong path nodes");

    return 0;
}
