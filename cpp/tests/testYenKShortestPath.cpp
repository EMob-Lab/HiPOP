#include "helpers.h"

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
    assertTrue(paths.size()==3, "Did not found 3 paths");


    assertTrue(paths[0].second==5, "First path cost not equal 5");
    assertTrue(paths[0].first==std::vector<std::string>{"C", "E", "F", "H"}, "First path nodes not equal C, E, F, H");
    assertTrue(paths[1].second==7, "Second path cost not equal 7");
    assertTrue(paths[1].first==std::vector<std::string>{"C", "E", "G", "H"}, "Second path nodes not equal C, E, G, H");
    assertTrue(paths[2].second==8, "Third path cost not equal 8");
    assertTrue(paths[2].first==std::vector<std::string>{"C", "D", "F", "H"}, "Third path nodes not equal C, D, F, H");


    return 0;
}
