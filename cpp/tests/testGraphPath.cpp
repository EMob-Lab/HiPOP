#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/create.h>
#include <hipop/graph_path.h>
#include <hipop/string_util.h>

#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace hipop;


static void testCase(std::string_view test_case_name, const std::function<void()> &test_fun) {
    std::cout << "TEST CASE " << test_case_name << std::endl;
    test_fun();
}


static void checkPathInvariant(const OrientedGraphPath &path, std::string_view errorMessageSuffix = "") {

    for (std::size_t i = 0; i < path.size(); ++i) {
        assertTrue(path.node(i) == path.link(i)->mup, StrCat(
            "node[i] == link[i]->up() invariant broken for i=", std::to_string(i), errorMessageSuffix
        ));

        assertTrue(path.node(i + 1) == path.link(i)->mdown, StrCat(
            "node[i + 1] == link[i]->down() invariant broken for i=", std::to_string(i), errorMessageSuffix
        ));
    }

    assertTrue(path.origin() == path.node(0), StrCat(
        "origin == node[0] invariant broken", errorMessageSuffix
    ));
    assertTrue(path.destination() == path.node(path.size()), StrCat(
        "destination == node[N] invariant broken", errorMessageSuffix
    ));
}


static void test_EmptyPath() {

    const OrientedGraph G = simple_graph(true);
    const Node *A = G.mnodes.at("A");

    OrientedGraphPath path = makeEmptyPath(G, "A");

    assertTrue(path.empty(), "Path should be empty");
    assertTrue(path.size() == 0, "Path size should be 0");
    assertTrue(path.origin() == A, "Wrong origin");
    assertTrue(path.destination() == A, "Destination should be identical to origin");
    assertTrue(path.node(0) == A, "Wrong node #0");
    assertTrue(path.nodeId(0) == "A", "Wrong node ID #0");
    assertTrue(!path.nodes().empty(), "Node range should never be empty");
    assertTrue(path.nodes().size() == 1, "Path should have exactly 1 node");
    assertTrue(path.nodeIds() == std::vector<std::string>{"A"}, "Wrong node IDs");
    assertTrue(path.links().empty(), "Link list should be empty");
    assertTrue(path.links().size() == 0, "Link list size should be 0");
    assertTrue(path.linkIds() == std::vector<std::string>{}, "Link ID list should be empty");
    checkPathInvariant(path);

    assertThrows<std::out_of_range>([&path]() {
        [[maybe_unused]] auto node = path.node(1);
    }, "node(1) should throw");

    assertThrows<std::out_of_range>([&path]() {
        [[maybe_unused]] auto link = path.link(0);
    }, "link(0) should throw");
}


static void test_MakePath() {

    const OrientedGraph G = simple_graph(true);

    OrientedGraphPath path = makePath(G, {"A_B", "B_E", "E_I"});

    assertTrue(!path.empty(), "Path should not be empty");
    assertTrue(path.size() == 3, "Path size should be 3");
    assertTrue(path.origin() == G.mnodes.at("A"), "Wrong origin");
    assertTrue(path.destination() == G.mnodes.at("I"), "Wrong destination");
    assertTrue(!path.nodes().empty(), "Node range should never be empty");
    assertTrue(path.nodes().size() == 4, "Path should have exactly 4 nodes");
    assertTrue(!path.links().empty(), "Link list should not be empty");
    assertTrue(path.links().size() == 3, "Link list size should be 3");
    checkPathInvariant(path);

    const std::vector<std::string> expectedNodeIds = {"A", "B", "E", "I"};
    const std::vector<std::string> expectedLinkIds = {"A_B", "B_E", "E_I"};
    assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs");
    assertTrue(path.linkIds() == expectedLinkIds, "Wrong link IDs");

    for (std::size_t i = 0; i < path.size(); ++i) {
        assertTrue(path.link(i) == G.mlinks.at(expectedLinkIds[i]), "Wrong link");
        assertTrue(path.linkId(i) == expectedLinkIds[i], "Wrong link ID");
    }
    for (std::size_t i = 0; i <= path.size(); ++i) {
        assertTrue(path.node(i) == G.mnodes.at(expectedNodeIds[i]), "Wrong node");
        assertTrue(path.nodeId(i) == expectedNodeIds[i], "Wrong node ID");
    }

    assertThrows<std::out_of_range>([&path]() {
        [[maybe_unused]] auto node = path.node(4);
    }, "node(size() + 1) should throw");

    assertThrows<std::out_of_range>([&path]() {
        [[maybe_unused]] auto link = path.link(3);
    }, "link(size()) should throw");
}


static void test_MakePathErrors() {

    const OrientedGraph G = simple_graph(true);

    assertThrows<std::invalid_argument>([&G]() {
        [[maybe_unused]] auto path = makePath(G, {});
    }, "Empty link list should throw");

    assertThrows<std::invalid_argument>([&G]() {
        [[maybe_unused]] auto path = makePath(G, {"A_B", "D_E"});
    }, "Non-consecutive links should throw");

    assertThrows<std::invalid_argument>([&G]() {
        [[maybe_unused]] auto path = makePath(G, {"B_E", "A_B"});
    }, "Links in wrong order should throw");
}


static void test_Add() {

    const OrientedGraph G = simple_graph(true);

    OrientedGraphPath path = makeEmptyPath(G, "A");
    checkPathInvariant(path, " (initial state)");
    path.add(G.mlinks.at("A_D"));
    checkPathInvariant(path, " (after 1 insertion)");
    path.add(G.mlinks.at("D_E"));
    checkPathInvariant(path, " (after 2 insertions)");

    assertTrue(path == makePath(G, {"A_D", "D_E"}), "Path should be equal to the one built by makePath");
    assertTrue(path.destination() == G.mnodes.at("E"), "Wrong destination");

    // Adding a link that does not start from the current destination must throw, and leave the path unchanged.
    assertThrows<std::invalid_argument>([&]() {
        path.add(G.mlinks.at("B_C"));
    }, "Inconsistent link should throw");
    assertTrue(path.size() == 2, "Path should be unchanged after a failed add");
    assertTrue(path.destination() == G.mnodes.at("E"), "Destination should be unchanged after a failed add");
    checkPathInvariant(path, " (after rejected insertion)");
}


static void test_Constructor() {

    const OrientedGraph G = simple_graph(true);
    const Node *A = G.mnodes.at("A");
    const Node *B = G.mnodes.at("B");
    const Link *A_B = G.mlinks.at("A_B");
    const Link *B_C = G.mlinks.at("B_C");
    const Link *E_F = G.mlinks.at("E_F");

    OrientedGraphPath path(A, {A_B, B_C});
    assertTrue(path.linkIds() == std::vector<std::string>{"A_B", "B_C"}, "Wrong link IDs");
    checkPathInvariant(path, " (for path A -> C)");

    OrientedGraphPath emptyPath(A, {});
    assertTrue(emptyPath == makeEmptyPath(G, "A"), "Empty link list should give an empty path");
    checkPathInvariant(emptyPath, " (for empty path A -> A)");

    assertThrows<std::invalid_argument>([&]() {
        OrientedGraphPath(B, {A_B});
    }, "Origin inconsistent with the first link should throw");

    assertThrows<std::invalid_argument>([&]() {
        OrientedGraphPath(A, {A_B, E_F});
    }, "Non-consecutive links should throw");
}


static void test_Equality() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path1 = makePath(G, {"A_B", "B_C"});
    const OrientedGraphPath path2 = makePath(G, {"A_B", "B_C"});
    const OrientedGraphPath path3 = makePath(G, {"A_B", "B_E"});
    const OrientedGraphPath path4 = makePath(G, {"A_B"});
    const OrientedGraphPath emptyPathA = makeEmptyPath(G, "A");
    const OrientedGraphPath emptyPathB = makeEmptyPath(G, "B");

    assertTrue(path1 == path2, "Identical paths should be equal");
    assertTrue(path1 != path3, "Paths with different links should differ");
    assertTrue(path1 != path4, "Paths with different sizes should differ");
    assertTrue(emptyPathA != emptyPathB, "Empty paths with different origins should differ");

    // Paths are compared by Node/Link identity: identical IDs in a copy of the graph do not make equal paths.
    const OrientedGraph copyG = G;
    const OrientedGraphPath path1InCopyG = makePath(copyG, {"A_B", "B_C"});
    assertTrue(path1 != path1InCopyG, "Paths built on different graph instances should differ");
}


static void test_NodeIteration() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path = makePath(G, {"A_B", "B_E", "E_F"});
    const std::vector<std::string> expectedNodeIds = {"A", "B", "E", "F"};
    const std::vector<std::string> expectedReversedNodeIds = {"F", "E", "B", "A"};

    // Forward iteration.
    std::vector<std::string> nodeIds;
    for (const Node *node : path.nodes()) {
        nodeIds.emplace_back(node->mid);
    }
    assertTrue(nodeIds == expectedNodeIds, "Wrong forward iteration");
    assertTrue(path.nodes().size() == 4, "Wrong number of nodes");

    // Reverse iteration.
    std::vector<std::string> reversedNodeIds;
    for (auto it = path.nodes().rbegin(); it != path.nodes().rend(); ++it) {
        reversedNodeIds.emplace_back((*it)->mid);
    }
    assertTrue(reversedNodeIds == expectedReversedNodeIds, "Wrong reverse iteration");

    // Post-increment/decrement return the previous position.
    auto it = path.nodes().begin();
    assertTrue(*it++ == G.mnodes.at("A") && *it == G.mnodes.at("B"), "Wrong post-increment");
    assertTrue(*it-- == G.mnodes.at("B") && *it == G.mnodes.at("A"), "Wrong post-decrement");

    // Empty path: a single node is visited.
    const OrientedGraphPath emptyPath = makeEmptyPath(G, "C");
    std::vector<const Node *> nodes(emptyPath.nodes().begin(), emptyPath.nodes().end());
    assertTrue(nodes == std::vector<const Node *>{G.mnodes.at("C")}, "Wrong iteration on empty path");
}


static void test_PathWithLoop() {

    // Visiting the same nodes and links several times is allowed.
    const OrientedGraph G = graph_with_loop();
    const OrientedGraphPath path = makePath(G, {"A_L1", "L1_L2", "L2_L1", "L1_L2", "L2_C"});

    const std::vector<std::string> expectedNodeIds = {"A", "L1", "L2", "L1", "L2", "C"};
    assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs");
    assertTrue(path.size() == 5, "Wrong size");
    assertTrue(path.link(1) == path.link(3), "Repeated link should be the same object");
    checkPathInvariant(path);
}


static void test_PathWithSelfLink() {

    // Same as test_PathWithLoop, but with a self-link.
    const OrientedGraph G = graph_with_self_link();
    const OrientedGraphPath path = makePath(G, {"A_L", "L_L", "L_L", "L_B"});

    const std::vector<std::string> expectedNodeIds = {"A", "L", "L", "L", "B"};
    assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs with self-link");
    assertTrue(path.size() == 4, "Wrong size");
    assertTrue(path.link(1) == path.link(2), "Repeated link should be the same object");
    checkPathInvariant(path);
}


static void test_PathWithExcludedMovement() {

    // OrientedGraphPath does not enforce exclude-movement rules: a path with a forbidden transition can be built.
    const OrientedGraph G = graph_with_exclude_movements();

    // Sanity check: the transition A -> B2 -> C is indeed forbidden in the graph.
    const auto &excludeMovements = G.mnodes.at("B2")->mexclude_movements;
    assertTrue(
        excludeMovements.count("A") > 0 && excludeMovements.at("A").count("C") > 0,
        "Transition A -> B2 -> C should be forbidden in the test graph"
    );

    const std::vector<std::string> expectedNodeIds = {"A", "B2", "C"};
    const std::vector<std::string> expectedLinkIds = {"A_B2", "B2_C"};

    // With makePath.
    {
        const OrientedGraphPath path = makePath(G, {"A_B2", "B2_C"});
        assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs with makePath(..)");
        assertTrue(path.linkIds() == expectedLinkIds, "Wrong link IDs with makePath(..)");
    }

    // With add.
    {
        OrientedGraphPath path = makeEmptyPath(G, "A");
        path.add(G.mlinks.at("A_B2"));
        path.add(G.mlinks.at("B2_C"));
        assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs with add(..)");
        assertTrue(path.linkIds() == expectedLinkIds, "Wrong link IDs with add(..)");
    }
}


int testGraphPath(int, char**) {
    testCase("Empty path", test_EmptyPath);
    testCase("makePath", test_MakePath);
    testCase("makePath errors", test_MakePathErrors);
    testCase("add", test_Add);
    testCase("Constructor", test_Constructor);
    testCase("Equality", test_Equality);
    testCase("Node iteration", test_NodeIteration);
    testCase("Path with loop", test_PathWithLoop);
    testCase("Path with self-link", test_PathWithSelfLink);
    testCase("Path with excluded movement", test_PathWithExcludedMovement);
    std::cout << "DONE" << std::endl;
    return 0;
}
