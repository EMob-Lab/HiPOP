#include "helpers.h"

#include <hipop/graph.h>

#include <memory>


std::unique_ptr<hipop::OrientedGraph> makeInitialGraph() {

    auto G = std::make_unique<hipop::OrientedGraph>();

    G->AddNode("0", 0, 0);
    G->AddNode("1", 1, 0);
    G->AddNode("2", 1, 1);
    G->AddNode("3", 0, 1);

    G->AddLink("0_1", "0", "1", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G->AddLink("1_2", "1", "2", 1, {{"PersonalVehicle", {{"time", 13}}}}, "CAR");
    G->AddLink("0_3", "0", "3", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");
    G->AddLink("3_2", "3", "2", 1, {{"PersonalVehicle", {{"time", 12}}}}, "CAR");

    return G;
}


void test_CopyConstructor() {

    auto G = makeInitialGraph();

    // Object copy in C++ is a deep copy, thus both instances are expected to be independent.
    hipop::OrientedGraph copyG = *G;
    G->mlinks["0_1"]->mcosts["PersonalVehicle"]["time"] = 42;

    assertTrue(
        G->mlinks["0_1"]->mcosts["PersonalVehicle"]["time"] != copyG.mlinks["0_1"]->mcosts["PersonalVehicle"]["time"],
        "[Copy constructor] Cost of original and copy should be different on link '0_1'");
    assertTrue(
        G->mlinks["0_3"]->mcosts["PersonalVehicle"]["time"] == copyG.mlinks["0_3"]->mcosts["PersonalVehicle"]["time"],
        "[Copy constructor] Cost of original and copy should be the same on link '0_3'");
    assertTrue(
        G->mlinks["0_3"] != copyG.mlinks["0_3"],
        "[Copy constructor] Pointers on links must be different");
}


void test_CopyAssignmentOperator() {

    auto G = makeInitialGraph();

    hipop::OrientedGraph copyG;
    copyG.AddNode("abcd", 0, 0);

    assertTrue(
        copyG.mnodes.find("abcd") != copyG.mnodes.end(),
        "[AddNode] Node 'abcd' should exist after being added");

    // Assignment operator is expected to perform a deep copy as well,
    // and also to erase the pre-existing content of the target instance.
    copyG = *G;
    G->mlinks["0_1"]->mcosts["PersonalVehicle"]["time"] = 42;

    assertTrue(copyG.mnodes.find("abcd") == copyG.mnodes.end(),
        "[Copy assignment operator] Node 'abcd' should not exist after copying from G");
    assertTrue(
        G->mlinks["0_1"]->mcosts["PersonalVehicle"]["time"] != copyG.mlinks["0_1"]->mcosts["PersonalVehicle"]["time"],
        "[Copy assignment operator] Cost of original and copy should be different on link '0_1'");
    assertTrue(
        G->mlinks["0_3"]->mcosts["PersonalVehicle"]["time"] == copyG.mlinks["0_3"]->mcosts["PersonalVehicle"]["time"],
        "[Copy assignment operator] Cost of original and copy should be the same on link '0_3'");
    assertTrue(
        G->mlinks["0_3"] != copyG.mlinks["0_3"],
        "[Copy assignment operator] Pointers on links must be different");
}


int testCopyGraph(int, char**) {
    test_CopyConstructor();
    test_CopyAssignmentOperator();
    return 0;
}
