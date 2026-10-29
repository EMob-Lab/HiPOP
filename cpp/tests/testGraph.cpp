// Same test scenarios as in python/tests/test_Graph.py, except for the parts
// that have no equivalent between the C++ and Python APIs.
// WARNING: any modification in this file should be reflected in the python test as well.

#include "helpers.h"

#include <hipop/graph.h>

#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace hipop;


/**
 * Graph used by the accessor and range tests:
 *
 * ```
 *        10          20
 *   A --------> B --------> C      D (isolated)
 * ```
 */
static OrientedGraph makeGraph() {
    OrientedGraph G;
    G.AddNode("A", 0, 0);
    G.AddNode("B", 1, 0);
    G.AddNode("C", 2, 0);
    G.AddNode("D", 3, 3);
    G.AddLink("A_B", "A", "B", 10, {{"CAR", {{"time", 1}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 20, {{"BUS", {{"time", 2}}}}, "BusLayer");
    return G;
}


/**
 * IDs of the elements enumerated by the given range (a multiset, so that duplicates would be detected).
 */
template<typename Range>
static std::multiset<std::string> collectIds(const Range &range) {
    std::multiset<std::string> result;
    for (const auto *element : range) {
        result.insert(element->mid);
    }
    return result;
}


static void test_EmptyGraph() {

    OrientedGraph G;
    const OrientedGraph &cG = G;

    assertTrue(cG.nodeCount() == 0, "[EmptyGraph] nodeCount() should be 0");
    assertTrue(cG.linkCount() == 0, "[EmptyGraph] linkCount() should be 0");
    assertTrue(!cG.hasNode("A"), "[EmptyGraph] hasNode() should be false");
    assertTrue(!cG.hasLink("A_B"), "[EmptyGraph] hasLink() should be false");
    assertTrue(cG.findNode("A") == nullptr, "[EmptyGraph] findNode() should return nullptr");
    assertTrue(cG.findLink("A_B") == nullptr, "[EmptyGraph] findLink() should return nullptr");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] auto node = cG.node("A");
    }, "[EmptyGraph] node() should throw");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Link *link = cG.link("A_B");
    }, "[EmptyGraph] link() should throw");

    assertTrue(cG.nodes().begin() == cG.nodes().end(), "[EmptyGraph] Read-only node range should be empty");
    assertTrue(G.nodes().begin() == G.nodes().end(), "[EmptyGraph] Read-write node range should be empty");
    assertTrue(cG.links().begin() == cG.links().end(), "[EmptyGraph] Read-only link range should be empty");
    assertTrue(G.links().begin() == G.links().end(), "[EmptyGraph] Read-write link range should be empty");
}


static void test_NodeAccessors() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    assertTrue(cG.nodeCount() == 4, "[NodeAccessors] nodeCount() should be 4");
    assertTrue(cG.hasNode("A") && cG.hasNode("D"), "[NodeAccessors] hasNode() should be true for existing nodes");
    assertTrue(!cG.hasNode("Z"), "[NodeAccessors] hasNode() should be false for unknown nodes");

    // node()
    const Node *a = cG.node("A");
    assertTrue(
        a != nullptr && a->mid == "A" && a->mposition[0] == 0 && a->mposition[1] == 0,
        "[NodeAccessors] node() should return node 'A'"
    );
    assertTrue(G.node("A") == a, "[NodeAccessors] Read-only and read-write node() should return the same node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Node *node = cG.node("Z");
    }, "[NodeAccessors] Read-only node() should throw for unknown nodes");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Node *node = G.node("Z");
    }, "[NodeAccessors] Read-write node() should throw for unknown nodes");

    // findNode()
    assertTrue(cG.findNode("A") == a, "[NodeAccessors] Read-only findNode() should return node 'A'");
    assertTrue(G.findNode("A") == a, "[NodeAccessors] Read-write findNode() should return node 'A'");
    assertTrue(cG.findNode("Z") == nullptr, "[NodeAccessors] Read-only findNode() should return nullptr for unknown nodes");
    assertTrue(G.findNode("Z") == nullptr, "[NodeAccessors] Read-write findNode() should return nullptr for unknown nodes");

    // Modification through the read-write accessors.
    G.node("B")->mlabel = "Modified";
    assertTrue(cG.node("B")->mlabel == "Modified", "[NodeAccessors] Modification through node() should be visible");
    G.findNode("C")->mlabel = "Modified";
    assertTrue(cG.node("C")->mlabel == "Modified", "[NodeAccessors] Modification through findNode() should be visible");

    // Accessors must stay consistent after a deletion.
    G.DeleteNode("B");
    assertTrue(cG.nodeCount() == 3, "[NodeAccessors] nodeCount() should be 3 after deletion");
    assertTrue(!cG.hasNode("B"), "[NodeAccessors] hasNode() should be false after deletion");
    assertTrue(cG.findNode("B") == nullptr, "[NodeAccessors] findNode() should return nullptr after deletion");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Node *node = cG.node("B");
    }, "[NodeAccessors] node() should throw after deletion");
    assertTrue(
        collectIds(cG.nodes()) == std::multiset<std::string>{"A", "C", "D"},
        "[NodeAccessors] Range should not contain deleted node"
    );
}


static void test_LinkAccessors() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    assertTrue(cG.linkCount() == 2, "[LinkAccessors] linkCount() should be 2");
    assertTrue(cG.hasLink("A_B") && cG.hasLink("B_C"), "[LinkAccessors] hasLink() should be true for existing links");
    assertTrue(!cG.hasLink("Z"), "[LinkAccessors] hasLink() should be false for unknown links");

    // link()
    const Link *ab = cG.link("A_B");
    assertTrue(
        ab != nullptr && ab->mid == "A_B" && ab->mlabel == "CarLayer" && ab->mlength == 10,
        "[LinkAccessors] link() should return link 'A_B'"
    );
    assertTrue(ab->mup == cG.node("A") && ab->mdown == cG.node("B"), "[LinkAccessors] Link 'A_B' should go from 'A' to 'B'");
    assertTrue(G.link("A_B") == ab, "[LinkAccessors] Read-only and read-write link() should return the same link");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Link *link = cG.link("Z");
    }, "[LinkAccessors] Read-only link() should throw for unknown links");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Link *link = G.link("Z");
    }, "[LinkAccessors] Read-write link() should throw for unknown links");

    // findLink()
    assertTrue(cG.findLink("A_B") == ab, "[LinkAccessors] Read-only findLink() should return link 'A_B'");
    assertTrue(G.findLink("A_B") == ab, "[LinkAccessors] Read-write findLink() should return link 'A_B'");
    assertTrue(cG.findLink("Z") == nullptr, "[LinkAccessors] Read-only findLink() should return nullptr for unknown links");
    assertTrue(G.findLink("Z") == nullptr, "[LinkAccessors] Read-write findLink() should return nullptr for unknown links");

    // Modification through the read-write accessors.
    G.link("A_B")->updateCosts({{"CAR", {{"time", 42}}}});
    assertTrue(
        cG.link("A_B")->mcosts.at("CAR").at("time") == 42,
        "[LinkAccessors] Modification through link() should be visible"
    );
    G.findLink("B_C")->updateCosts({{"BUS", {{"time", 43}}}});
    assertTrue(
        cG.link("B_C")->mcosts.at("BUS").at("time") == 43,
        "[LinkAccessors] Modification through findLink() should be visible"
    );

    // Accessors must stay consistent after a deletion.
    G.DeleteLink("A_B");
    assertTrue(cG.linkCount() == 1, "[LinkAccessors] linkCount() should be 1 after deletion");
    assertTrue(!cG.hasLink("A_B"), "[LinkAccessors] hasLink() should be false after deletion");
    assertTrue(cG.findLink("A_B") == nullptr, "[LinkAccessors] findLink() should return nullptr after deletion");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Link *link = cG.link("A_B");
    }, "[LinkAccessors] link() should throw after deletion");
    assertTrue(
        collectIds(cG.links()) == std::multiset<std::string>{"B_C"},
        "[LinkAccessors] Range should not contain deleted link"
    );
}


static void test_NodeRanges() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    static_assert(std::is_same_v<decltype(cG.nodes()), OrientedGraph::ConstNodeRange>);
    static_assert(std::is_same_v<decltype(G.nodes()), OrientedGraph::NodeRange>);
    static_assert(std::is_same_v<decltype(*cG.nodes().begin()), const Node *>);
    static_assert(std::is_same_v<decltype(*G.nodes().begin()), Node *>);
    static_assert(std::is_convertible_v<OrientedGraph::NodeRange, OrientedGraph::ConstNodeRange>);
    static_assert(!std::is_convertible_v<OrientedGraph::ConstNodeRange, OrientedGraph::NodeRange>);

    const std::multiset<std::string> expected{"A", "B", "C", "D"};
    assertTrue(collectIds(cG.nodes()) == expected, "[NodeRanges] Read-only range should enumerate each node once");
    assertTrue(collectIds(G.nodes()) == expected, "[NodeRanges] Read-write range should enumerate each node once");

    for (const Node *node : cG.nodes()) {
        assertTrue(node == cG.node(node->mid), "[NodeRanges] Read-only range should enumerate the nodes of the graph");
    }

    // Modification through the read-write range.
    for (Node *node : G.nodes()) {
        node->mlabel = "Label" + node->mid;
    }
    for (const Node *node : cG.nodes()) {
        assertTrue(
            node->mlabel == "Label" + node->mid,
            "[NodeRanges] Modification through the read-write range should be visible"
        );
    }

    // Conversion from read-write to read-only, and mixed comparisons.
    OrientedGraph::ConstNodeRange converted = G.nodes();
    assertTrue(collectIds(converted) == expected, "[NodeRanges] Converted range should enumerate each node once");
    OrientedGraph::ConstNodeIterator cit = G.nodes().begin();
    OrientedGraph::NodeIterator it = G.nodes().begin();
    assertTrue(cit == it && it == cit && !(cit != it) && !(it != cit), "[NodeRanges] Mixed comparisons should work");
    assertTrue(
        G.nodes().end() == cG.nodes().end(),
        "[NodeRanges] Read-only and read-write ranges should have the same end"
    );

    // Post-increment.
    OrientedGraph::ConstNodeIterator first = cG.nodes().begin();
    OrientedGraph::ConstNodeIterator previous = first++;
    assertTrue(
        previous == cG.nodes().begin() && previous != first,
        "[NodeRanges] Post-increment should return the previous position"
    );
}


static void test_LinkRanges() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    static_assert(std::is_same_v<decltype(cG.links()), OrientedGraph::ConstLinkRange>);
    static_assert(std::is_same_v<decltype(G.links()), OrientedGraph::LinkRange>);
    static_assert(std::is_same_v<decltype(*cG.links().begin()), const Link *>);
    static_assert(std::is_same_v<decltype(*G.links().begin()), Link *>);
    static_assert(std::is_convertible_v<OrientedGraph::LinkRange, OrientedGraph::ConstLinkRange>);
    static_assert(!std::is_convertible_v<OrientedGraph::ConstLinkRange, OrientedGraph::LinkRange>);
    static_assert(!std::is_convertible_v<OrientedGraph::LinkIterator, OrientedGraph::ConstNodeIterator>);

    const std::multiset<std::string> expected{"A_B", "B_C"};
    assertTrue(collectIds(cG.links()) == expected, "[LinkRanges] Read-only range should enumerate each link once");
    assertTrue(collectIds(G.links()) == expected, "[LinkRanges] Read-write range should enumerate each link once");

    for (const Link *link : cG.links()) {
        assertTrue(link == cG.link(link->mid), "[LinkRanges] Read-only range should enumerate the links of the graph");
    }

    // Modification through the read-write range.
    for (Link *link : G.links()) {
        link->updateCosts({{"CAR", {{"time", 100}}}});
    }
    for (const Link *link : cG.links()) {
        assertTrue(
            link->mcosts.at("CAR").at("time") == 100,
            "[LinkRanges] Modification through the read-write range should be visible"
        );
    }

    // Conversion from read-write to read-only, and mixed comparisons.
    OrientedGraph::ConstLinkRange converted = G.links();
    assertTrue(collectIds(converted) == expected, "[LinkRanges] Converted range should enumerate each link once");
    OrientedGraph::ConstLinkIterator cit = G.links().begin();
    OrientedGraph::LinkIterator it = G.links().begin();
    assertTrue(cit == it && it == cit && !(cit != it) && !(it != cit), "[LinkRanges] Mixed comparisons should work");
    assertTrue(
        G.links().end() == cG.links().end(),
        "[LinkRanges] Read-only and read-write ranges should have the same end"
    );
}


static void test_AddNode() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");

    G.AddNode("E", 4, 4, "NewNode");
    const Node *e = G.node("E");
    assertTrue(G.nodeCount() == 5, "[AddNode] nodeCount() should be 5 after insertion");
    assertTrue(
        e->mid == "E" && e->mposition[0] == 4 && e->mposition[1] == 4 && e->mlabel == "NewNode",
        "[AddNode] node() should return the inserted node"
    );

    // Duplicated ID.
    assertThrows<std::invalid_argument>([&]() {
        G.AddNode("A", 5, 5, "Duplicate");
    }, "[AddNode] AddNode() should throw for a duplicated node ID");
    assertTrue(G.nodeCount() == 5, "[AddNode] nodeCount() should be unchanged after a failed insertion");
    assertTrue(G.node("A") == a, "[AddNode] Existing node should not be replaced by a failed insertion");
    assertTrue(
        a->mposition[0] == 0 && a->mposition[1] == 0 && a->mlabel.empty(),
        "[AddNode] Existing node should not be modified by a failed insertion"
    );
    assertTrue(
        a->madj.size() == 1 && a->madj.at("B") == G.link("A_B"),
        "[AddNode] Links of the existing node should not be modified by a failed insertion"
    );
}


static void test_AddLink() {

    OrientedGraph G = makeGraph();
    const Link *ab = G.link("A_B");

    G.AddLink("C_D", "C", "D", 30, {{"CAR", {{"time", 3}}}}, "CarLayer");
    const Link *cd = G.link("C_D");
    assertTrue(G.linkCount() == 3, "[AddLink] linkCount() should be 3 after insertion");
    assertTrue(
        cd->mid == "C_D" && cd->mup == G.node("C") && cd->mdown == G.node("D") && cd->mlength == 30,
        "[AddLink] link() should return the inserted link"
    );
    assertTrue(
        G.node("C")->madj.at("D") == cd && G.node("D")->mradj.at("C") == cd,
        "[AddLink] Inserted link should be registered in the adjacency of its nodes"
    );

    // Duplicated ID.
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("A_B", "D", "A", 40, {{"CAR", {{"time", 4}}}}, "Duplicate");
    }, "[AddLink] AddLink() should throw for a duplicated link ID");
    assertTrue(G.linkCount() == 3, "[AddLink] linkCount() should be unchanged after a failed insertion");
    assertTrue(G.link("A_B") == ab, "[AddLink] Existing link should not be replaced by a failed insertion");
    assertTrue(
        ab->mup == G.node("A") && ab->mdown == G.node("B") && ab->mlength == 10 && ab->mlabel == "CarLayer",
        "[AddLink] Existing link should not be modified by a failed insertion"
    );
    assertTrue(
        G.node("D")->madj.empty() && G.node("A")->mradj.empty(),
        "[AddLink] Failed insertion should not modify the adjacency of the nodes"
    );

    // Unknown upstream/downstream node.
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("Z_A", "Z", "A", 50, {{"CAR", {{"time", 5}}}}, "CarLayer");
    }, "[AddLink] AddLink() should throw for an unknown upstream node");
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("A_Z", "A", "Z", 50, {{"CAR", {{"time", 5}}}}, "CarLayer");
    }, "[AddLink] AddLink() should throw for an unknown downstream node");
    assertTrue(G.linkCount() == 3, "[AddLink] linkCount() should be unchanged after a failed insertion");
    assertTrue(!G.hasLink("Z_A") && !G.hasLink("A_Z"), "[AddLink] Failed insertion should not add any link");
    assertTrue(!G.hasNode("Z"), "[AddLink] Failed insertion should not add any node");
    assertTrue(
        G.node("A")->madj.size() == 1 && G.node("A")->mradj.empty(),
        "[AddLink] Failed insertion should not modify the adjacency of the nodes"
    );
}


static void test_DeleteLink() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");
    const Node *b = G.node("B");
    const Link *bc = G.link("B_C");

    G.DeleteLink("A_B");
    assertTrue(G.linkCount() == 1 && !G.hasLink("A_B"), "[DeleteLink] Link 'A_B' should be removed");
    assertTrue(G.nodeCount() == 4, "[DeleteLink] Nodes should not be removed");
    assertTrue(
        G.node("A") == a && G.node("B") == b && G.link("B_C") == bc,
        "[DeleteLink] Remaining elements should not be relocated"
    );
    assertTrue(
        a->madj.empty() && b->mradj.empty(),
        "[DeleteLink] Deleted link should be unregistered from the adjacency of its nodes"
    );
    assertTrue(
        b->madj.size() == 1 && b->madj.at("C") == bc && G.node("C")->mradj.at("B") == bc,
        "[DeleteLink] Adjacency of the other links should not be modified"
    );

    // Unknown ID, or link already deleted: no-op.
    G.DeleteLink("Z");
    G.DeleteLink("A_B");
    assertTrue(
        G.linkCount() == 1 && G.nodeCount() == 4,
        "[DeleteLink] Deleting an unknown link should not modify the graph"
    );

    // A link with the same ID and endpoints can be added again.
    G.AddLink("A_B", "A", "B", 15, {{"CAR", {{"time", 5}}}}, "CarLayer");
    assertTrue(
        G.link("A_B")->mlength == 15 && a->madj.at("B") == G.link("A_B") && b->mradj.at("A") == G.link("A_B"),
        "[DeleteLink] Deleted link should be re-insertable"
    );
}


static void test_DeleteNode() {

    // Additional links so that node 'B' has several entrance and exit links, plus a self-loop
    // (which appears both in its entrance and exit links):
    //
    //           Self-link
    //             on B
    //        10    (*)    20
    //   A --------> B <-------> C
    //               |
    //               +---------> D
    OrientedGraph G = makeGraph();
    G.AddLink("C_B", "C", "B", 30, {{"CAR", {{"time", 3}}}}, "CarLayer");
    G.AddLink("B_D", "B", "D", 40, {{"CAR", {{"time", 4}}}}, "CarLayer");
    G.AddLink("B_B", "B", "B", 5, {{"CAR", {{"time", 1}}}}, "CarLayer"); // Self-link
    const Node *a = G.node("A");
    const Node *c = G.node("C");
    const Node *d = G.node("D");

    G.DeleteNode("B");
    assertTrue(G.nodeCount() == 3 && !G.hasNode("B"), "[DeleteNode] Node 'B' should be removed");
    assertTrue(G.linkCount() == 0, "[DeleteNode] All the links from or to node 'B' should be removed");
    assertTrue(
        G.node("A") == a && G.node("C") == c && G.node("D") == d,
        "[DeleteNode] Remaining nodes should not be relocated"
    );
    assertTrue(
        a->madj.empty() && a->mradj.empty() && c->madj.empty() && c->mradj.empty()
            && d->madj.empty() && d->mradj.empty(),
        "[DeleteNode] Deleted links should be unregistered from the adjacency of the remaining nodes"
    );

    // Unknown ID, or node already deleted: no-op.
    G.DeleteNode("Z");
    G.DeleteNode("B");
    assertTrue(G.nodeCount() == 3, "[DeleteNode] Deleting an unknown node should not modify the graph");

    // Isolated node.
    G.AddLink("A_C", "A", "C", 50, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.DeleteNode("D");
    assertTrue(G.nodeCount() == 2 && !G.hasNode("D"), "[DeleteNode] Isolated node 'D' should be removed");
    assertTrue(
        G.linkCount() == 1 && a->madj.at("C") == G.link("A_C"),
        "[DeleteNode] Links not adjacent to the deleted node should not be modified"
    );

    // A node with the same ID can be added again, and linked.
    G.AddNode("B", 5, 5);
    G.AddLink("A_B", "A", "B", 60, {{"CAR", {{"time", 6}}}}, "CarLayer");
    assertTrue(
        G.node("B")->mposition[0] == 5 && G.node("B")->mradj.at("A") == G.link("A_B"),
        "[DeleteNode] Deleted node should be re-insertable"
    );
}


// No Python equivalent: checks C++-specific guarantees on the address of the nodes and links.
static void test_PointerStability() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");
    const Link *ab = G.link("A_B");

    // Enough insertions to trigger several rehashes.
    for (int i = 0; i < 1000; ++i) {
        G.AddNode("N" + std::to_string(i), i, i);
        G.AddLink("L" + std::to_string(i), "A", "N" + std::to_string(i), 1, {});
    }
    assertTrue(G.node("A") == a && G.link("A_B") == ab, "[PointerStability] Insertions should not relocate elements");
    assertTrue(ab->mup == a && a->madj.at("B") == ab, "[PointerStability] Relationships should survive insertions");

    // Move construction/assignment transfer the elements without relocating them.
    OrientedGraph moved(std::move(G));
    assertTrue(moved.node("A") == a && moved.link("A_B") == ab, "[PointerStability] Move should not relocate elements");
    OrientedGraph assigned;
    assigned = std::move(moved);
    assertTrue(
        assigned.node("A") == a && assigned.link("A_B") == ab,
        "[PointerStability] Move-assignment should not relocate elements"
    );

    // Copies are deep, with relationships pointing to the elements of the copy.
    OrientedGraph copy(assigned);
    const Node *copyA = copy.node("A");
    const Link *copyAB = copy.link("A_B");
    assertTrue(copyA != a && copyAB != ab, "[PointerStability] Copy should create new elements");
    assertTrue(
        copyAB->mup == copyA && copyAB->mdown == copy.node("B") && copyA->madj.at("B") == copyAB,
        "[PointerStability] Relationships in the copy should point to the elements of the copy"
    );
    OrientedGraph copyAssigned = makeGraph();
    copyAssigned = assigned;
    assertTrue(
        copyAssigned.nodeCount() == assigned.nodeCount() && copyAssigned.linkCount() == assigned.linkCount(),
        "[PointerStability] Copy-assignment should replace the content"
    );
    assertTrue(
        copyAssigned.link("A_B")->mup == copyAssigned.node("A"),
        "[PointerStability] Relationships in the copy-assigned graph should point to its own elements"
    );
}


static void test_CopyConstructor() {

    OrientedGraph G = makeGraph();

    // Copy is deep, thus both instances are expected to be independent.
    OrientedGraph copyG(G);
    G.link("A_B")->updateCosts({{"CAR", {{"time", 42}}}});

    assertTrue(
        G.link("A_B")->mcosts.at("CAR").at("time") != copyG.link("A_B")->mcosts.at("CAR").at("time"),
        "[CopyConstructor] Cost of original and copy should be different on link 'A_B'"
    );
    assertTrue(
        G.link("B_C")->mcosts.at("BUS").at("time") == copyG.link("B_C")->mcosts.at("BUS").at("time"),
        "[CopyConstructor] Cost of original and copy should be the same on link 'B_C'"
    );
    assertTrue(G.link("B_C") != copyG.link("B_C"), "[CopyConstructor] Links must be different objects");
}


// No Python equivalent: Python has no assignment operator.
static void test_CopyAssignmentOperator() {

    OrientedGraph G = makeGraph();

    OrientedGraph copyG;
    copyG.AddNode("Z", 0, 0);

    // Assignment operator is expected to perform a deep copy as well,
    // and also to erase the pre-existing content of the target instance.
    copyG = G;
    G.link("A_B")->updateCosts({{"CAR", {{"time", 42}}}});

    assertTrue(!copyG.hasNode("Z"), "[CopyAssignmentOperator] Pre-existing node 'Z' should be erased by the copy");
    assertTrue(
        G.link("A_B")->mcosts.at("CAR").at("time") != copyG.link("A_B")->mcosts.at("CAR").at("time"),
        "[CopyAssignmentOperator] Cost of original and copy should be different on link 'A_B'"
    );
    assertTrue(
        G.link("B_C")->mcosts.at("BUS").at("time") == copyG.link("B_C")->mcosts.at("BUS").at("time"),
        "[CopyAssignmentOperator] Cost of original and copy should be the same on link 'B_C'"
    );
    assertTrue(G.link("B_C") != copyG.link("B_C"), "[CopyAssignmentOperator] Links must be different objects");
}


static void test_Exits() {

    OrientedGraph G;
    G.AddNode("A", 0, 0);

    std::unordered_map<std::string, std::set<std::string> > excludeMovements;
    excludeMovements["A"] = {"C"};
    G.AddNode("B", 2, 5, "", excludeMovements);

    G.AddNode("C", 12., 43.);

    G.AddNode("D", 435, 345);
    G.AddLink("A_B", "A", "B", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.AddLink("B_D", "B", "D", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");

    std::vector<const Link*> exits = G.node("B")->getExits("A");

    assertTrue(exits.size() == 1, "[Exits] getExits() should return one link");
    assertTrue(exits[0]->mdown->mid == "D", "[Exits] Exit link should go to 'D'");
}


int testGraph(int, char**) {
    test_EmptyGraph();
    test_NodeAccessors();
    test_LinkAccessors();
    test_NodeRanges();
    test_LinkRanges();
    test_AddNode();
    test_AddLink();
    test_DeleteLink();
    test_DeleteNode();
    test_PointerStability();
    test_CopyConstructor();
    test_CopyAssignmentOperator();
    test_Exits();
    return 0;
}
