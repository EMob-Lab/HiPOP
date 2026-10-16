#include "helpers.h"
#include "sample_graphs.h"

#include <hipop/create.h>
#include <hipop/graph_path.h>
#include <hipop/string_util.h>

#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
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
    assertTrue(path.originId() == "A", "Wrong origin ID");
    assertTrue(path.destination() == A, "Destination should be identical to origin");
    assertTrue(path.destinationId() == "A", "Wrong destination ID");
    assertTrue(path.node(0) == A, "Wrong node #0");
    assertTrue(path.nodeId(0) == "A", "Wrong node ID #0");
    assertTrue(!path.nodes().empty(), "Node range should never be empty");
    assertTrue(path.nodes().size() == 1, "Path should have exactly 1 node");
    assertTrue(path.nodeIds() == std::vector<std::string>{"A"}, "Wrong node IDs");
    assertTrue(path.links().empty(), "Link list should be empty");
    assertTrue(path.links().size() == 0, "Link list size should be 0");
    assertTrue(path.linkIds() == std::vector<std::string>{}, "Link ID list should be empty");
    assertTrue(!path.hasLoop(), "Empty path should have no loop");
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
    assertTrue(path.originId() == "A", "Wrong origin ID");
    assertTrue(path.destination() == G.mnodes.at("I"), "Wrong destination");
    assertTrue(path.destinationId() == "I", "Wrong destination ID");
    assertTrue(!path.nodes().empty(), "Node range should never be empty");
    assertTrue(path.nodes().size() == 4, "Path should have exactly 4 nodes");
    assertTrue(!path.links().empty(), "Link list should not be empty");
    assertTrue(path.links().size() == 3, "Link list size should be 3");
    assertTrue(!path.hasLoop(), "Path should have no loop");
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

    assertThrows<std::invalid_argument>([&G]() {
        [[maybe_unused]] auto path = makePath(G, {"A_B", "I'm not a link ID"});
    }, "Unknown link ID should throw");

    assertThrows<std::invalid_argument>([&G]() {
        [[maybe_unused]] auto path = makeEmptyPath(G, "I'm not a node ID");
    }, "Unknown node ID should throw");
}


static void test_AddLink() {

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


static void test_AddPath() {

    const OrientedGraph G = simple_graph(true);

    { // Base case
        OrientedGraphPath path = makePath(G, {"A_B"});
        path.add(makePath(G, {"B_E", "E_F"}));
        checkPathInvariant(path, " (base case)");
        assertTrue(path == makePath(G, {"A_B", "B_E", "E_F"}), "Wrong path after concatenation (base case)");
    }

    { // path + empty path
        OrientedGraphPath path = makePath(G, {"A_B", "B_E"});
        path.add(makeEmptyPath(G, "E"));
        checkPathInvariant(path, " (path + empty path)");
        assertTrue(path == makePath(G, {"A_B", "B_E"}), "Wrong path after concatenation (path + empty path)");
    }

    { // empty path + path
        OrientedGraphPath emptyPath = makeEmptyPath(G, "A");
        emptyPath.add(makePath(G, {"A_D", "D_E"}));
        checkPathInvariant(emptyPath, " (empty path + path)");
        assertTrue(emptyPath == makePath(G, {"A_D", "D_E"}), "Wrong path concatenation (empty path + path)");
    }

    { // empty path + empty path
        OrientedGraphPath emptyPath = makeEmptyPath(G, "A");
        emptyPath.add(makeEmptyPath(G, "A"));
        checkPathInvariant(emptyPath, " (empty path + empty path)");
        assertTrue(emptyPath == makeEmptyPath(G, "A"), "Wrong path concatenation (empty path + empty path)");
    }

    // Adding a path that does not start from the current destination must throw, and leave the path unchanged.
    {
        OrientedGraphPath path = makePath(G, {"A_B", "B_E"});
        assertThrows<std::invalid_argument>([&]() {
            path.add(makePath(G, {"B_C"}));
        }, "Inconsistent non-empty path should throw");
        assertThrows<std::invalid_argument>([&]() {
            path.add(makeEmptyPath(G, "A"));
        }, "Inconsistent empty path should throw");
        assertTrue(path == makePath(G, {"A_B", "B_E"}), "Path should be unchanged after a failed add");
    }
}


static void test_AddPathToItself() {

    const OrientedGraph G = graph_with_loop();

    { // Loop
        OrientedGraphPath path = makePath(G, {"L1_L2", "L2_L1"});
        path.add(path);
        checkPathInvariant(path, " (loop)");
        assertTrue(path == makePath(G, {"L1_L2", "L2_L1", "L1_L2", "L2_L1"}), "Wrong path after self-concatenation (loop)");
    }

    { // Empty path
        OrientedGraphPath path = makeEmptyPath(G, "A");
        path.add(path);
        checkPathInvariant(path, " (empty path)");
        assertTrue(path == makeEmptyPath(G, "A"), "Wrong path after self-concatenation (empty path)");
    }

    // Adding a non-loop path to itself must throw, and leave the path unchanged.
    {
        OrientedGraphPath path = makePath(G, {"A_L1", "L1_L2"});
        assertThrows<std::invalid_argument>([&]() {
            path.add(path);
        }, "Self-concatenation of a non-loop path should throw");
        assertTrue(path == makePath(G, {"A_L1", "L1_L2"}), "Path should be unchanged after a failed add");
    }
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

    // Paths are compared by Node/Link identity: identical IDs in a copy of the graph do not result in equal paths.
    const OrientedGraph copyG = G; // NOLINT (performance-unnecessary-copy-initialization)
    const OrientedGraphPath path1InCopyG = makePath(copyG, {"A_B", "B_C"});
    assertTrue(path1 != path1InCopyG, "Paths built on different graph instances should differ");
}


static void test_Hash() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path1 = makePath(G, {"A_B", "B_C"});
    const OrientedGraphPath path2 = makePath(G, {"A_B", "B_C"});
    const OrientedGraphPath path3 = makePath(G, {"A_B", "B_E"});
    const OrientedGraphPath path4 = makePath(G, {"A_B"});
    const OrientedGraphPath emptyPathA = makeEmptyPath(G, "A");
    const OrientedGraphPath emptyPathB = makeEmptyPath(G, "B");
    const std::hash<OrientedGraphPath> hasher;

    assertTrue(hasher(path1) == hasher(path2), "Identical paths should have the same hash");

    // Different paths may collide in theory, but not on these simple cases.
    assertTrue(hasher(path1) != hasher(path3), "Paths with different links should have different hashes");
    assertTrue(hasher(path1) != hasher(path4), "Paths with different sizes should have different hashes");
    assertTrue(hasher(emptyPathA) != hasher(emptyPathB), "Empty paths with different origins should have different hashes");

    // Paths are hashed by Node/Link identity: identical IDs in a copy of the graph do not result in equal hashes.
    const OrientedGraph copyG = G; // NOLINT (performance-unnecessary-copy-initialization)
    const OrientedGraphPath path1InCopyG = makePath(copyG, {"A_B", "B_C"});
    assertTrue(hasher(path1) != hasher(path1InCopyG), "Paths built on different graph instances should have different hashes");

    const std::unordered_set<OrientedGraphPath> paths = { path1, path2, path3 }; // path1 and path2 are identical
    assertTrue(paths.size() == 2, "Identical paths should be deduplicated in an unordered_set");
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
    assertTrue(path.hasLoop(), "Path should have a loop");
    checkPathInvariant(path);

    const OrientedGraphPath loopStartingOnL1 = makePath(G, {"L1_L2", "L2_L1"});
    const OrientedGraphPath loopStartingOnL2 = makePath(G, {"L2_L1", "L1_L2"});
    assertTrue(loopStartingOnL1 != loopStartingOnL2, "Loop L1 -> L1 is not the same as loop L2 -> L2");
    assertTrue(loopStartingOnL1.hasLoop(), "Loop L1 -> L1 should have a loop");
    assertTrue(!makePath(G, {"A_L1", "L1_L2", "L2_C"}).hasLoop(), "Path through L1 and L2 only once should have no loop");
}


static void test_PathWithSelfLink() {

    // Same as test_PathWithLoop, but with a self-link.
    const OrientedGraph G = graph_with_self_link();
    const OrientedGraphPath path = makePath(G, {"A_L", "L_L", "L_L", "L_B"});

    const std::vector<std::string> expectedNodeIds = {"A", "L", "L", "L", "B"};
    assertTrue(path.nodeIds() == expectedNodeIds, "Wrong node IDs with self-link");
    assertTrue(path.size() == 4, "Wrong size");
    assertTrue(path.link(1) == path.link(2), "Repeated link should be the same object");
    assertTrue(path.hasLoop(), "Path with self-link should have a loop");
    checkPathInvariant(path);

    const OrientedGraphPath emptyPathOnL = makeEmptyPath(G, "L");
    const OrientedGraphPath oneLoopOnL = makePath(G, {"L_L"});
    const OrientedGraphPath twoLoopsOnL = makePath(G, {"L_L", "L_L"});
    assertTrue(oneLoopOnL != emptyPathOnL, "Loop L -> L is not the same as empty path on L");
    assertTrue(oneLoopOnL != twoLoopsOnL, "Loop L -> L is not the same as double-loop L -> L");
    assertTrue(oneLoopOnL.hasLoop(), "Self-link L -> L should be a loop");
    assertTrue(!emptyPathOnL.hasLoop(), "Empty path on L should have no loop");
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


static void test_SubPath() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path = makePath(G, {"A_B", "B_E", "E_F"});

    // Default link count: up to the end of the path.
    assertTrue(path.subPath(0) == path, "subPath(0) should be the whole path");
    assertTrue(path.subPath(1) == makePath(G, {"B_E", "E_F"}), "Wrong subPath(1)");
    assertTrue(path.subPath(3) == makeEmptyPath(G, "F"), "subPath(size()) should be the empty path on destination");

    // Explicit link count.
    assertTrue(path.subPath(0, 0) == makeEmptyPath(G, "A"), "Wrong subPath(0, 0)");
    assertTrue(path.subPath(1, 0) == makeEmptyPath(G, "B"), "Wrong subPath(1, 0)");
    assertTrue(path.subPath(0, 2) == makePath(G, {"A_B", "B_E"}), "Wrong subPath(0, 2)");
    assertTrue(path.subPath(1, 1) == makePath(G, {"B_E"}), "Wrong subPath(1, 1)");
    assertTrue(path.subPath(1, 2) == makePath(G, {"B_E", "E_F"}), "Wrong subPath(1, 2)");

    // Link count exceeding the remaining number of links is truncated.
    assertTrue(path.subPath(1, 10) == makePath(G, {"B_E", "E_F"}), "Wrong subPath(1, 10)");
    assertTrue(path.subPath(3, 10) == makeEmptyPath(G, "F"), "Wrong subPath(3, 10)");

    for (std::size_t pos = 0; pos <= path.size(); ++pos) {
        for (std::size_t linkCount = 0; linkCount <= path.size() + 1; ++linkCount) {
            checkPathInvariant(path.subPath(pos, linkCount), StrCat(
                " (for subPath(", std::to_string(pos), ", ", std::to_string(linkCount), "))"
            ));
        }
    }

    // Sub-path of an empty path.
    const OrientedGraphPath emptyPath = makeEmptyPath(G, "C");
    assertTrue(emptyPath.subPath(0) == emptyPath, "subPath(0) of an empty path should be the path itself");

    // Expecting `pos <= size()`.
    assertThrows<std::out_of_range>([&]() {
        [[maybe_unused]] auto p = path.subPath(4);
    }, "subPath(size() + 1) should throw");
    assertThrows<std::out_of_range>([&]() {
        [[maybe_unused]] auto p = emptyPath.subPath(1, 0);
    }, "subPath(1, 0) of an empty path should throw");
}


static void test_StartsWith() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path = makePath(G, {"A_B", "B_E", "E_F"});

    assertTrue(path.startsWith(path), "A path should start with itself");
    assertTrue(path.startsWith(makePath(G, {"A_B"})), "Path should start with [A_B]");
    assertTrue(path.startsWith(makePath(G, {"A_B", "B_E"})), "Path should start with [A_B, B_E]");
    assertTrue(path.startsWith(makeEmptyPath(G, "A")), "Path should start with the empty path on its origin");

    assertTrue(!path.startsWith(makeEmptyPath(G, "B")), "Path should not start with an empty path on another node");
    assertTrue(!path.startsWith(makePath(G, {"B_E"})), "Path should not start with [B_E]");
    assertTrue(!path.startsWith(makePath(G, {"A_B", "B_C"})), "Path should not start with [A_B, B_C]");

    assertTrue(!makePath(G, {"A_B"}).startsWith(path), "A path should not start with a longer path");

    // Empty current path.
    const OrientedGraphPath emptyPath = makeEmptyPath(G, "A");
    assertTrue(emptyPath.startsWith(emptyPath), "An empty path should start with itself");
    assertTrue(!emptyPath.startsWith(makeEmptyPath(G, "B")), "Empty path should not start with an empty path on another node");
    assertTrue(!emptyPath.startsWith(makePath(G, {"A_B"})), "Empty path should not start with a non-empty path");

    // Consistency with subPath.
    for (std::size_t linkCount = 0; linkCount <= path.size(); ++linkCount) {
        assertTrue(
            path.startsWith(path.subPath(0, linkCount)),
            StrCat("Path should start with subPath(0, ", std::to_string(linkCount), ")")
        );
    }
}


static void test_EndsWith() {

    const OrientedGraph G = simple_graph(true);
    const OrientedGraphPath path = makePath(G, {"A_B", "B_E", "E_F"});

    assertTrue(path.endsWith(path), "A path should end with itself");
    assertTrue(path.endsWith(makePath(G, {"E_F"})), "Path should end with [E_F]");
    assertTrue(path.endsWith(makePath(G, {"B_E", "E_F"})), "Path should end with [B_E, E_F]");
    assertTrue(path.endsWith(makeEmptyPath(G, "F")), "Path should end with the empty path on its destination");

    assertTrue(!path.endsWith(makeEmptyPath(G, "E")), "Path should not end with an empty path on another node");
    assertTrue(!path.endsWith(makePath(G, {"B_E"})), "Path should not end with [B_E]");
    assertTrue(!path.endsWith(makePath(G, {"D_E", "E_F"})), "Path should not end with [D_E, E_F]");

    assertTrue(!makePath(G, {"E_F"}).endsWith(path), "A path should not end with a longer path");

    // Empty current path.
    const OrientedGraphPath emptyPath = makeEmptyPath(G, "F");
    assertTrue(emptyPath.endsWith(emptyPath), "An empty path should end with itself");
    assertTrue(!emptyPath.endsWith(makeEmptyPath(G, "E")), "Empty path should not end with an empty path on another node");
    assertTrue(!emptyPath.endsWith(makePath(G, {"E_F"})), "Empty path should not end with a non-empty path");

    // Consistency with subPath.
    for (std::size_t pos = 0; pos <= path.size(); ++pos) {
        assertTrue(
            path.endsWith(path.subPath(pos)),
            StrCat("Path should end with subPath(", std::to_string(pos), ")")
        );
    }
}


static void test_StreamOperator() {

    const OrientedGraph G = simple_graph(true);

    {
        std::ostringstream oss;
        oss << makePath(G, {"A_B", "B_E", "E_I"});
        assertTrue(oss.str() == "from=A links=[A_B B_E E_I] to=I", "Wrong output for non-empty path");
    }
    {
        std::ostringstream oss;
        oss << makeEmptyPath(G, "C");
        assertTrue(oss.str() == "from=C links=[] to=C", "Wrong output for empty path");
    }
}


int testGraphPath(int, char**) {
    testCase("Empty path", test_EmptyPath);
    testCase("makePath", test_MakePath);
    testCase("makePath errors", test_MakePathErrors);
    testCase("Add link", test_AddLink);
    testCase("Add path", test_AddPath);
    testCase("Add path to itself", test_AddPathToItself);
    testCase("Constructor", test_Constructor);
    testCase("Equality", test_Equality);
    testCase("Hash", test_Hash);
    testCase("Node iteration", test_NodeIteration);
    testCase("Path with loop", test_PathWithLoop);
    testCase("Path with self-link", test_PathWithSelfLink);
    testCase("Path with excluded movement", test_PathWithExcludedMovement);
    testCase("subPath", test_SubPath);
    testCase("startsWith", test_StartsWith);
    testCase("endsWith", test_EndsWith);
    testCase("Stream operator", test_StreamOperator);
    std::cout << "DONE" << std::endl;
    return 0;
}
