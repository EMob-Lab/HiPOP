#include "helpers.h"

#include <hipop/graph.h>

#include <string>

using namespace hipop;


int testGraphMerge(int, char**)
{
    OrientedGraph G1;
    G1.AddNode("a", 0, 0);
    G1.AddNode("b", 2, 5, "", {{"a", {"c"}}});
    G1.AddNode("c", 12, 43);
    G1.AddNode("d", 435, 345);

    G1.AddLink("a_b", "a", "b", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G1.AddLink("b_c", "b", "c", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G1.AddLink("b_d", "b", "d", 12, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");


    OrientedGraph G2;

    G2.AddNode("f", 39, 3);
    G2.AddNode("y", 42, 0);

    G2.AddLink("f_y", "f", "y", 22, {{"PersonalVehicle", {{"time", 22}}}}, "CAR");


    OrientedGraph G3;

    G3.AddNode("h", 39, 3);

    auto mergeG = mergeOrientedGraph({&G1, &G2, &G3});


    assertTrue(mergeG.nodeCount()==7, "Merge graph does not have 7 nodes");
    assertTrue(mergeG.linkCount()==4, "Merge graph does not have 4 links");


    return 0;
}
