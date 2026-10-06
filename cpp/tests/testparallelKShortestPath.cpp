#include "helpers.h"

#include <hipop/graph.h>
#include <hipop/shortest_path.h>

using namespace hipop;


int testparallelKShortestPath(int, char**)
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



    std::vector<std::string> origins = {"0", "0", "0", "0"};
    std::vector<std::string> destinations = {"3", "3", "3", "3"};
    std::vector<int> kPaths = {4, 4, 4, 4};
    std::vector<std::unordered_map<std::string, std:: string> > vecMapLabelCosts = {
        {{"CAR", "PersonalVehicle"}},
        {{"CAR", "PersonalVehicle"}},
        {{"CAR", "PersonalVehicle"}},
        {{"CAR", "PersonalVehicle"}}
    };

    auto paths = parallelKShortestPath(G, origins, destinations, "time", vecMapLabelCosts, {}, 0.1, 0.95, 1.2, 10, kPaths, 4);


    return 0;
}
