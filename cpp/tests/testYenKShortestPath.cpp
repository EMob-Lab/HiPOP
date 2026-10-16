#include "helpers.h"

#include <hipop/create.h>
#include <hipop/graph.h>
#include <hipop/shortest_path.h>

using namespace hipop;


int testYenKShortestPath(int, char**)
{
    OrientedGraph G;

    G.AddNode("C", 0, 0);
    G.AddNode("D", 1, 0);
    G.AddNode("F", 2, 0);
    G.AddNode("E", 1, -1);
    G.AddNode("G", 2, -1);
    G.AddNode("H", 3, -1);

    G.AddLink("C_D", "C", "D", 1, {{"PersonalVehicle", {{"time", 3}}}}, "CAR");
    G.AddLink("C_E", "C", "E", 1, {{"PersonalVehicle", {{"time", 2}}}}, "CAR");
    G.AddLink("D_F", "D", "F", 1, {{"PersonalVehicle", {{"time", 4}}}}, "CAR");
    G.AddLink("E_D", "E", "D", 1, {{"PersonalVehicle", {{"time", 1}}}}, "CAR");
    G.AddLink("E_F", "E", "F", 1, {{"PersonalVehicle", {{"time", 2}}}}, "CAR");
    G.AddLink("E_G", "E", "G", 1, {{"PersonalVehicle", {{"time", 3}}}}, "CAR");
    G.AddLink("F_G", "F", "G", 1, {{"PersonalVehicle", {{"time", 2}}}}, "CAR");
    G.AddLink("F_H", "F", "H", 1, {{"PersonalVehicle", {{"time", 1}}}}, "CAR");
    G.AddLink("G_H", "G", "H", 1, {{"PersonalVehicle", {{"time", 2}}}}, "CAR");


    auto paths = YenKShortestPath(G, "C", "H", "time", {}, {{"CAR", "PersonalVehicle"}}, 3);

    assertTrue(paths.size() == 3, "Did not found 3 paths");
    assertEqualPaths(paths[0], PathCost{makePath(G, {"C_E", "E_F", "F_H"}), 5}, "Wrong path #0");
    assertEqualPaths(paths[1], PathCost{makePath(G, {"C_E", "E_G", "G_H"}), 7}, "Wrong path #1");
    assertEqualPaths(paths[2], PathCost{makePath(G, {"C_D", "D_F", "F_H"}), 8}, "Wrong path #2");

    return 0;
}
