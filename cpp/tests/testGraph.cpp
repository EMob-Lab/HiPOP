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
    static_assert(std::is_same_v<decltype(*cG.nodes().begin()), const Node *const &>);
    static_assert(std::is_same_v<decltype(*G.nodes().begin()), Node *const &>);
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
    static_assert(std::is_same_v<decltype(*cG.links().begin()), const Link *const &>);
    static_assert(std::is_same_v<decltype(*G.links().begin()), Link *const &>);
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
    test_Exits();
    return 0;
}
