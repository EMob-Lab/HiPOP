#include "helpers.h"

#include <hipop/graph.h>

#include <unordered_map>
#include <set>
#include <string>
#include <memory>
#include <iostream>


// Same test scenario as in python/tests/test_Graph.py.
// WARNING: any modification in this test should be reflected in the python test as well.
int testGraph(int, char**)
{
    hipop::OrientedGraph *G = new hipop::OrientedGraph();
    G->AddNode("a", 0, 0);

    std::unordered_map<std::string, std::set<std::string> > excludeMovements;
    excludeMovements["a"] = {"c"};
    G->AddNode("b", 2, 5, "", excludeMovements);

    hipop::Node *newNode = new hipop::Node("c", 12., 43.);
    G->AddNode(newNode);

    G->AddNode("d", 435, 345);
    G->AddLink("a_b", "a", "b", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G->AddLink("b_c", "b", "c", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G->AddLink("b_d", "b", "d", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");

    std::vector<hipop::Link*> exits = G->mnodes["b"]->getExits("a");

    assertTrue(exits.size()==1, "Exits does not return one link");
    assertTrue(exits[0]->mdown->mid=="d", "Node should be d");

    return 0;
}
