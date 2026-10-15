#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

using namespace hipop;


int testKShortestPath(int, char**)
{
    OrientedGraph G;

    G.AddNode("0", 0, 0);
    G.AddNode("1", 1, 1);
    G.AddNode("2", 1, -1);
    G.AddNode("3", 2, 0);
    G.AddNode("4", 2, 1);

    G.AddLink("0_1", "0", "1", 1, {{"PersonalVehicle", {{"time", 14}}}}, "CAR");
    G.AddLink("1_3", "1", "3", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G.AddLink("0_2", "0", "2", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G.AddLink("2_3", "2", "3", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G.AddLink("0_3", "0", "3", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G.AddLink("0_4", "0", "4", 11, {{"PersonalVehicle", {{"time", 14}}}}, "CAR");
    G.AddLink("4_3", "4", "3", 11, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");

    auto paths = KShortestPath(G, "0", "3", "time", {}, {{"CAR", "PersonalVehicle"}}, 5, 100, 2.2, 10, 3, false);

    assertTrue(paths.size() == 3, "Did not found 3 paths");
    assertEqualPaths(paths[0], pathCost{{"0", "3"}, 12}, "Wrong path #0");
    assertEqualPaths(paths[1], pathCost{{"0", "2", "3"}, 24}, "Wrong path #1");
    assertEqualPaths(paths[2], pathCost{{"0", "1", "3"}, 26}, "Wrong path #2");

    return 0;
}
