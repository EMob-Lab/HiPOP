// Same test scenarios as in python/tests/test_Graph.py, except for the parts
// that have no equivalent between the C++ and Python APIs.
// WARNING: any modification in this file should be reflected in the python test as well.

#include "helpers.h"

#include <hipop/graph.h>

#include <cstddef>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace hipop;


/**
 * Graph used by the tests (figures correspond to link lengths):
 *
 * ```
 *        10        30, 40, 99 (3 parallel links)
 *   A --------> B ============> E
 *               | <------------
 *            20 |       25            x D (isolated)
 *               v
 *               C (*) Self-loop on C
 * ```
 */
static OrientedGraph makeGraph() {
    OrientedGraph G;
    G.AddNode("A", 0, 0);
    G.AddNode("B", 1, 0);
    G.AddNode("C", 1, 2);
    G.AddNode("D", 8, 1);
    G.AddNode("E", 3, 0);
    G.AddLink("A_B", "A", "B", 10, {{"CAR", {{"time", 1}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 20, {{"BUS", {{"time", 2}}}}, "BusLayer");
    G.AddLink("C_C", "C", "C", 0, {{"WALK", {{"time", 0.1}}}}, "TransitLayer");
    G.AddLink("B_E_1", "B", "E", 30, {{"CAR", {{"time", 3}}}}, "CarLayer");
    G.AddLink("B_E_2", "B", "E", 40, {{"BUS", {{"time", 4}}}}, "BusLayer");
    G.AddLink("B_E_3", "B", "E", 99, {{"WALK", {{"time", 9}}}}, "TransitLayer");
    G.AddLink("E_B", "E", "B", 25, {{"CAR", {{"time", 2.5}}}}, "CarLayer");
    G.checkInternalConsistency();
    return G;
}


/**
 * IDs of the elements enumerated by the given range (a multiset, so that duplicates would be detected).
 */
template<typename Range>
static std::multiset<std::string> collectIds(const Range &range) {
    std::multiset<std::string> result;
    for (const auto *element : range) {
        result.insert(element->id());
    }
    return result;
}


/**
 * Elements enumerated by the given range (a multiset, so that duplicates would be detected).
 */
template<typename Range>
static auto collectElements(const Range &range) {
    using Element = std::remove_pointer_t<typename std::iterator_traits<typename Range::iterator>::value_type>;
    return std::multiset<const Element *>(range.begin(), range.end());
}


static void test_EmptyGraph() {

    OrientedGraph G;
    const OrientedGraph &cG = G;
    G.checkInternalConsistency();

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

    assertTrue(cG.nodeCount() == 5, "[NodeAccessors] nodeCount() should be 5");
    assertTrue(cG.hasNode("A") && cG.hasNode("D"), "[NodeAccessors] hasNode() should be true for existing nodes");
    assertTrue(!cG.hasNode("Z"), "[NodeAccessors] hasNode() should be false for unknown nodes");

    // node()
    const Node *a = cG.node("A");
    assertTrue(
        a != nullptr && a->id() == "A" && a->x() == 0 && a->y() == 0,
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

    // Accessors must stay consistent after a deletion.
    G.DeleteNode("B");
    G.checkInternalConsistency();
    assertTrue(cG.nodeCount() == 4, "[NodeAccessors] nodeCount() should be 4 after deletion");
    assertTrue(!cG.hasNode("B"), "[NodeAccessors] hasNode() should be false after deletion");
    assertTrue(cG.findNode("B") == nullptr, "[NodeAccessors] findNode() should return nullptr after deletion");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Node *node = cG.node("B");
    }, "[NodeAccessors] node() should throw after deletion");
    assertTrue(
        collectIds(cG.nodes()) == std::multiset<std::string>{"A", "C", "D", "E"},
        "[NodeAccessors] Range should not contain deleted node"
    );
}


static void test_LinkAccessors() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    assertTrue(cG.linkCount() == 7, "[LinkAccessors] linkCount() should be 7");
    assertTrue(cG.hasLink("A_B") && cG.hasLink("B_C"), "[LinkAccessors] hasLink() should be true for existing links");
    assertTrue(!cG.hasLink("Z"), "[LinkAccessors] hasLink() should be false for unknown links");

    // link()
    const Link *ab = cG.link("A_B");
    assertTrue(
        ab != nullptr && ab->id() == "A_B" && ab->label() == "CarLayer" && ab->length() == 10,
        "[LinkAccessors] link() should return link 'A_B'"
    );
    assertTrue(ab->up() == cG.node("A") && ab->down() == cG.node("B"), "[LinkAccessors] Link 'A_B' should go from 'A' to 'B'");
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
    G.link("A_B")->setCosts({{"CAR", {{"time", 42}}}});
    assertTrue(
        cG.link("A_B")->costs().at("CAR").at("time") == 42,
        "[LinkAccessors] Modification through link() should be visible"
    );
    G.findLink("B_C")->setCosts({{"BUS", {{"time", 43}}}});
    assertTrue(
        cG.link("B_C")->costs().at("BUS").at("time") == 43,
        "[LinkAccessors] Modification through findLink() should be visible"
    );

    // Accessors must stay consistent after a deletion.
    G.DeleteLink("A_B");
    G.checkInternalConsistency();
    assertTrue(cG.linkCount() == 6, "[LinkAccessors] linkCount() should be 6 after deletion");
    assertTrue(!cG.hasLink("A_B"), "[LinkAccessors] hasLink() should be false after deletion");
    assertTrue(cG.findLink("A_B") == nullptr, "[LinkAccessors] findLink() should return nullptr after deletion");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] const Link *link = cG.link("A_B");
    }, "[LinkAccessors] link() should throw after deletion");
    assertTrue(
        collectIds(cG.links()) == std::multiset<std::string>{"B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"},
        "[LinkAccessors] Range should not contain deleted link"
    );
}


static void test_NodeRanges() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    static_assert(std::is_same_v<decltype(cG.nodes()), OrientedGraph::ConstNodeRange>);
    static_assert(std::is_same_v<decltype(G.nodes()), OrientedGraph::NodeRange>);
    static_assert(std::is_same_v<OrientedGraph::ConstNodeRange::iterator::value_type, const Node *>);
    static_assert(std::is_same_v<OrientedGraph::NodeRange::iterator::value_type, Node *>);
    static_assert(std::is_convertible_v<OrientedGraph::NodeRange, OrientedGraph::ConstNodeRange>);
    static_assert(!std::is_convertible_v<OrientedGraph::ConstNodeRange, OrientedGraph::NodeRange>);

    const std::multiset<std::string> expected{"A", "B", "C", "D", "E"};
    assertTrue(collectIds(cG.nodes()) == expected, "[NodeRanges] Read-only range should enumerate each node once");
    assertTrue(collectIds(G.nodes()) == expected, "[NodeRanges] Read-write range should enumerate each node once");

    for (const Node *node : cG.nodes()) {
        assertTrue(node == cG.node(node->id()), "[NodeRanges] Read-only range should enumerate the nodes of the graph");
    }

    for (Node *node : G.nodes()) {
        assertTrue(node == cG.node(node->id()), "[NodeRanges] Read-write range should enumerate the nodes of the graph");
    }
}


static void test_LinkRanges() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;

    static_assert(std::is_same_v<decltype(cG.links()), OrientedGraph::ConstLinkRange>);
    static_assert(std::is_same_v<decltype(G.links()), OrientedGraph::LinkRange>);
    static_assert(std::is_same_v<OrientedGraph::ConstLinkRange::iterator::value_type, const Link *>);
    static_assert(std::is_same_v<OrientedGraph::LinkRange::iterator::value_type, Link *>);
    static_assert(std::is_convertible_v<OrientedGraph::LinkRange, OrientedGraph::ConstLinkRange>);
    static_assert(!std::is_convertible_v<OrientedGraph::ConstLinkRange, OrientedGraph::LinkRange>);

    const std::multiset<std::string> expected{"A_B", "B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"};
    assertTrue(collectIds(cG.links()) == expected, "[LinkRanges] Read-only range should enumerate each link once");
    assertTrue(collectIds(G.links()) == expected, "[LinkRanges] Read-write range should enumerate each link once");

    for (const Link *link : cG.links()) {
        assertTrue(link == cG.link(link->id()), "[LinkRanges] Read-only range should enumerate the links of the graph");
    }

    // Modification through the read-write range.
    for (Link *link : G.links()) {
        link->setCosts({{"CAR", {{"time", 100}}}});
    }
    for (const Link *link : cG.links()) {
        assertTrue(
            link->costs().at("CAR").at("time") == 100,
            "[LinkRanges] Modification through the read-write range should be visible"
        );
    }
}


static void test_AdjacentLinks() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");
    const Node *b = G.node("B");
    const Node *c = G.node("C");
    const Node *d = G.node("D");
    const Node *e = G.node("E");

    static_assert(std::is_same_v<decltype(b->inLinks()), Node::AdjacentLinkRange>);
    static_assert(std::is_same_v<decltype(b->outLinks()), Node::AdjacentLinkRange>);
    static_assert(std::is_same_v<Node::AdjacentLinkRange::iterator::value_type, const Link *>);

    assertTrue(
        a->outLinkCount() == 1 && collectIds(a->outLinks()) == std::multiset<std::string>{"A_B"},
        "[AdjacentLinks] outLinks() should enumerate the outgoing links"
    );
    assertTrue(
        b->inLinkCount() == 2 && collectIds(b->inLinks()) == std::multiset<std::string>{"A_B", "E_B"},
        "[AdjacentLinks] inLinks() should enumerate the incoming links"
    );
    assertTrue(
        b->outLinkCount() == 4
            && collectIds(b->outLinks()) == std::multiset<std::string>{"B_C", "B_E_1", "B_E_2", "B_E_3"},
        "[AdjacentLinks] outLinks() should enumerate the parallel outgoing links"
    );
    assertTrue(
        e->inLinkCount() == 3 && collectIds(e->inLinks()) == std::multiset<std::string>{"B_E_1", "B_E_2", "B_E_3"},
        "[AdjacentLinks] inLinks() should enumerate the parallel incoming links"
    );
    assertTrue(
        d->inLinkCount() == 0 && d->inLinks().empty() && d->outLinkCount() == 0 && d->outLinks().empty(),
        "[AdjacentLinks] Isolated node should have no adjacent link"
    );

    // A self-loop is both an incoming and an outgoing link.
    assertTrue(
        collectIds(c->inLinks()) == std::multiset<std::string>{"B_C", "C_C"}
            && collectIds(c->outLinks()) == std::multiset<std::string>{"C_C"},
        "[AdjacentLinks] Self-loop should be both an incoming and an outgoing link"
    );
}


static void test_ConnectingLinks() {

    OrientedGraph G = makeGraph();
    const OrientedGraph &cG = G;
    const Node *a = cG.node("A");
    const Node *b = cG.node("B");
    const Node *d = cG.node("D");
    const Node *e = cG.node("E");

    static_assert(std::is_same_v<decltype(cG.findConnectingLinks("B", "E")), OrientedGraph::ConnectingLinkRange>);
    static_assert(
        std::is_same_v<decltype(OrientedGraph::findConnectingLinks(b, e)), OrientedGraph::ConnectingLinkRange>
    );
    static_assert(std::is_same_v<OrientedGraph::ConnectingLinkRange::iterator::value_type, const Link *>);

    // Overloads with node IDs.
    assertTrue(
        cG.hasConnectingLink("B", "E") && cG.connectingLinkCount("B", "E") == 3
            && collectIds(cG.findConnectingLinks("B", "E")) == std::multiset<std::string>{"B_E_1", "B_E_2", "B_E_3"},
        "[ConnectingLinks] Parallel links from 'B' to 'E' should be found"
    );
    assertTrue(
        cG.hasConnectingLink("A", "B") && cG.connectingLinkCount("A", "B") == 1
            && collectIds(cG.findConnectingLinks("A", "B")) == std::multiset<std::string>{"A_B"},
        "[ConnectingLinks] Link from 'A' to 'B' should be found"
    );
    assertTrue(
        !cG.hasConnectingLink("B", "A") && cG.connectingLinkCount("B", "A") == 0
            && cG.findConnectingLinks("B", "A").begin() == cG.findConnectingLinks("B", "A").end(),
        "[ConnectingLinks] No link should be found from 'B' to 'A' (direction matters)"
    );
    assertTrue(
        cG.hasConnectingLink("E", "B") && cG.connectingLinkCount("E", "B") == 1
            && collectIds(cG.findConnectingLinks("E", "B")) == std::multiset<std::string>{"E_B"},
        "[ConnectingLinks] Link from 'E' to 'B' should be found (direction matters)"
    );
    assertTrue(
        cG.hasConnectingLink("C", "C") && cG.connectingLinkCount("C", "C") == 1
            && collectIds(cG.findConnectingLinks("C", "C")) == std::multiset<std::string>{"C_C"},
        "[ConnectingLinks] Self-loop on 'C' should be found"
    );
    assertTrue(
        !cG.hasConnectingLink("A", "C") && cG.connectingLinkCount("A", "C") == 0
            && cG.findConnectingLinks("A", "C").begin() == cG.findConnectingLinks("A", "C").end(),
        "[ConnectingLinks] No link should be found between non-adjacent nodes"
    );
    assertTrue(
        !cG.hasConnectingLink("A", "A") && cG.connectingLinkCount("A", "A") == 0
            && cG.findConnectingLinks("A", "A").begin() == cG.findConnectingLinks("A", "A").end(),
        "[ConnectingLinks] No link should be found from 'A' to 'A'"
    );

    // Overloads with nodes.
    assertTrue(
        OrientedGraph::hasConnectingLink(b, e) && OrientedGraph::connectingLinkCount(b, e) == 3
            && collectIds(OrientedGraph::findConnectingLinks(b, e))
                == std::multiset<std::string>{"B_E_1", "B_E_2", "B_E_3"},
        "[ConnectingLinks] Parallel links from 'B' to 'E' should be found (overloads with nodes)"
    );
    assertTrue(
        !OrientedGraph::hasConnectingLink(a, d) && OrientedGraph::connectingLinkCount(a, d) == 0
            && OrientedGraph::findConnectingLinks(a, d).begin() == OrientedGraph::findConnectingLinks(a, d).end(),
        "[ConnectingLinks] No link should be found from 'A' to isolated node 'D' (overloads with nodes)"
    );

    // Unknown node IDs.
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] bool result = cG.hasConnectingLink("Z", "A");
    }, "[ConnectingLinks] hasConnectingLink() should throw for an unknown upstream node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] bool result = cG.hasConnectingLink("A", "Z");
    }, "[ConnectingLinks] hasConnectingLink() should throw for an unknown downstream node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] std::size_t result = cG.connectingLinkCount("Z", "A");
    }, "[ConnectingLinks] connectingLinkCount() should throw for an unknown upstream node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] std::size_t result = cG.connectingLinkCount("A", "Z");
    }, "[ConnectingLinks] connectingLinkCount() should throw for an unknown downstream node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] auto result = cG.findConnectingLinks("Z", "A");
    }, "[ConnectingLinks] findConnectingLinks() should throw for an unknown upstream node");
    assertThrows<std::invalid_argument>([&]() {
        [[maybe_unused]] auto result = cG.findConnectingLinks("A", "Z");
    }, "[ConnectingLinks] findConnectingLinks() should throw for an unknown downstream node");
}


static void test_AddNode() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");

    G.AddNode("Z", 4, 4, "NewNode");
    G.checkInternalConsistency();
    const Node *z = G.node("Z");
    assertTrue(G.nodeCount() == 6, "[AddNode] nodeCount() should be 6 after insertion");
    assertTrue(
        z->id() == "Z" && z->x() == 4 && z->y() == 4 && z->label() == "NewNode",
        "[AddNode] node() should return the inserted node"
    );

    // Duplicated ID.
    assertThrows<std::invalid_argument>([&]() {
        G.AddNode("A", 5, 5, "Duplicate");
    }, "[AddNode] AddNode() should throw for a duplicated node ID");
    G.checkInternalConsistency();
    assertTrue(G.nodeCount() == 6, "[AddNode] nodeCount() should be unchanged after a failed insertion");
    assertTrue(G.node("A") == a, "[AddNode] Existing node should not be replaced by a failed insertion");
    assertTrue(
        a->x() == 0 && a->y() == 0 && a->label().empty(),
        "[AddNode] Existing node should not be modified by a failed insertion"
    );
    assertTrue(
        collectElements(a->outLinks()) == std::multiset<const Link *>{G.link("A_B")} && a->inLinks().empty(),
        "[AddNode] Links of the existing node should not be modified by a failed insertion"
    );
}


static void test_AddLink() {

    OrientedGraph G = makeGraph();
    const Link *ab = G.link("A_B");
    const Link *cc = G.link("C_C");

    G.AddLink("C_D", "C", "D", 30, {{"CAR", {{"time", 3}}}}, "CarLayer");
    G.checkInternalConsistency();
    const Link *cd = G.link("C_D");
    assertTrue(G.linkCount() == 8, "[AddLink] linkCount() should be 8 after insertion");
    assertTrue(
        cd->id() == "C_D" && cd->up() == G.node("C") && cd->down() == G.node("D") && cd->length() == 30,
        "[AddLink] link() should return the inserted link"
    );
    assertTrue(
        collectElements(G.node("C")->outLinks()) == std::multiset<const Link *>{cc, cd}
            && collectElements(G.node("D")->inLinks()) == std::multiset<const Link *>{cd},
        "[AddLink] Inserted link should be registered in the adjacency of its nodes"
    );
    assertTrue(
        collectElements(G.findConnectingLinks("C", "D")) == std::multiset<const Link *>{cd},
        "[AddLink] Inserted link should be found as a connecting link between its nodes"
    );

    // Duplicated ID.
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("A_B", "D", "A", 40, {{"CAR", {{"time", 4}}}}, "Duplicate");
    }, "[AddLink] AddLink() should throw for a duplicated link ID");
    G.checkInternalConsistency();
    assertTrue(G.linkCount() == 8, "[AddLink] linkCount() should be unchanged after a failed insertion");
    assertTrue(G.link("A_B") == ab, "[AddLink] Existing link should not be replaced by a failed insertion");
    assertTrue(
        ab->up() == G.node("A") && ab->down() == G.node("B") && ab->length() == 10 && ab->label() == "CarLayer",
        "[AddLink] Existing link should not be modified by a failed insertion"
    );
    assertTrue(
        G.node("D")->outLinks().empty() && G.node("A")->inLinks().empty(),
        "[AddLink] Failed insertion should not modify the adjacency of the nodes"
    );
    assertTrue(
        !G.hasConnectingLink("D", "A")
            && collectElements(G.findConnectingLinks("A", "B")) == std::multiset<const Link *>{ab},
        "[AddLink] Failed insertion should not modify the connecting links"
    );

    // Unknown upstream/downstream node.
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("Z_A", "Z", "A", 50, {{"CAR", {{"time", 5}}}}, "CarLayer");
    }, "[AddLink] AddLink() should throw for an unknown upstream node");
    assertThrows<std::invalid_argument>([&]() {
        G.AddLink("A_Z", "A", "Z", 50, {{"CAR", {{"time", 5}}}}, "CarLayer");
    }, "[AddLink] AddLink() should throw for an unknown downstream node");
    G.checkInternalConsistency();
    assertTrue(G.linkCount() == 8, "[AddLink] linkCount() should be unchanged after a failed insertion");
    assertTrue(!G.hasLink("Z_A") && !G.hasLink("A_Z"), "[AddLink] Failed insertion should not add any link");
    assertTrue(!G.hasNode("Z"), "[AddLink] Failed insertion should not add any node");
    assertTrue(
        G.node("A")->outLinkCount() == 1 && G.node("A")->inLinks().empty(),
        "[AddLink] Failed insertion should not modify the adjacency of the nodes"
    );
}


static void test_DeleteLink() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");
    const Node *b = G.node("B");
    const Link *bc = G.link("B_C");
    const Link *cc = G.link("C_C");
    const Link *be1 = G.link("B_E_1");
    const Link *be2 = G.link("B_E_2");
    const Link *be3 = G.link("B_E_3");
    const Link *eb = G.link("E_B");

    G.DeleteLink("A_B");
    G.checkInternalConsistency();
    assertTrue(G.linkCount() == 6 && !G.hasLink("A_B"), "[DeleteLink] Link 'A_B' should be removed");
    assertTrue(G.nodeCount() == 5, "[DeleteLink] Nodes should not be removed");
    assertTrue(
        G.node("A") == a && G.node("B") == b && G.link("B_C") == bc,
        "[DeleteLink] Remaining elements should not be relocated"
    );
    assertTrue(
        a->outLinks().empty() && collectElements(b->inLinks()) == std::multiset<const Link *>{eb},
        "[DeleteLink] Deleted link should be unregistered from the adjacency of its nodes"
    );
    assertTrue(
        collectElements(b->outLinks()) == std::multiset<const Link *>{bc, be1, be2, be3}
            && collectElements(G.node("C")->inLinks()) == std::multiset<const Link *>{bc, cc},
        "[DeleteLink] Adjacency of the other links should not be modified"
    );
    assertTrue(
        collectElements(G.findConnectingLinks("A", "B")).empty()
            && collectElements(G.findConnectingLinks("B", "E")) == std::multiset<const Link *>{be1, be2, be3},
        "[DeleteLink] Deleted link should not be found as a connecting link anymore, unlike the other links"
    );

    // Unknown ID, or link already deleted: no-op.
    G.DeleteLink("Z");
    G.DeleteLink("A_B");
    G.checkInternalConsistency();
    assertTrue(
        G.linkCount() == 6 && G.nodeCount() == 5,
        "[DeleteLink] Deleting an unknown link should not modify the graph"
    );

    // A link with the same ID and endpoints can be added again.
    G.AddLink("A_B", "A", "B", 15, {{"CAR", {{"time", 5}}}}, "CarLayer");
    G.checkInternalConsistency();
    assertTrue(
        G.link("A_B")->length() == 15 && collectElements(a->outLinks()) == std::multiset<const Link *>{G.link("A_B")}
            && collectElements(b->inLinks()) == std::multiset<const Link *>{G.link("A_B"), eb}
            && collectElements(G.findConnectingLinks("A", "B")) == std::multiset<const Link *>{G.link("A_B")},
        "[DeleteLink] Deleted link should be re-insertable"
    );
}


static void test_DeleteParallelLink() {

    OrientedGraph G = makeGraph();
    const Node *b = G.node("B");
    const Node *e = G.node("E");
    const Link *bc = G.link("B_C");
    const Link *be1 = G.link("B_E_1");
    const Link *be3 = G.link("B_E_3");
    const Link *eb = G.link("E_B");

    // Removal of one of the parallel links from B to E (the one in the middle of the insertion order).
    G.DeleteLink("B_E_2");
    G.checkInternalConsistency();
    assertTrue(G.linkCount() == 6 && !G.hasLink("B_E_2"), "[DeleteParallelLink] Link 'B_E_2' should be removed");
    assertTrue(
        G.link("B_E_1") == be1 && G.link("B_E_3") == be3,
        "[DeleteParallelLink] Other parallel links should not be relocated"
    );
    assertTrue(
        collectElements(b->outLinks()) == std::multiset<const Link *>{bc, be1, be3}
            && collectElements(e->inLinks()) == std::multiset<const Link *>{be1, be3},
        "[DeleteParallelLink] Other parallel links should remain in the adjacency of their nodes"
    );
    assertTrue(
        G.connectingLinkCount("B", "E") == 2
            && collectElements(G.findConnectingLinks("B", "E")) == std::multiset<const Link *>{be1, be3},
        "[DeleteParallelLink] Other parallel links should still be found as connecting links"
    );
    assertTrue(
        collectElements(G.findConnectingLinks("E", "B")) == std::multiset<const Link *>{eb},
        "[DeleteParallelLink] Link in the opposite direction should not be affected"
    );
}


static void test_DeleteNode() {

    OrientedGraph G = makeGraph();
    const Node *a = G.node("A");
    const Node *b = G.node("B");
    const Node *c = G.node("C");
    const Node *d = G.node("D");
    const Link *ab = G.link("A_B");
    const Link *bc = G.link("B_C");
    const Link *cc = G.link("C_C");

    G.DeleteNode("E");
    G.checkInternalConsistency();
    assertTrue(G.nodeCount() == 4 && !G.hasNode("E"), "[DeleteNode] Node 'E' should be removed");
    assertTrue(
        G.linkCount() == 3 && !G.hasLink("B_E_1") && !G.hasLink("B_E_2") && !G.hasLink("B_E_3") && !G.hasLink("E_B"),
        "[DeleteNode] All the links from or to node 'E' should be removed"
    );
    assertTrue(
        G.node("A") == a && G.node("B") == b && G.node("C") == c && G.node("D") == d
            && G.link("A_B") == ab && G.link("B_C") == bc && G.link("C_C") == cc,
        "[DeleteNode] Remaining elements should not be relocated"
    );
    assertTrue(
        collectElements(b->inLinks()) == std::multiset<const Link *>{ab}
            && collectElements(b->outLinks()) == std::multiset<const Link *>{bc},
        "[DeleteNode] Deleted links should be unregistered from the adjacency of the remaining nodes"
    );
    assertTrue(
        collectElements(a->outLinks()) == std::multiset<const Link *>{ab}
            && collectElements(c->inLinks()) == std::multiset<const Link *>{bc, cc}
            && collectElements(c->outLinks()) == std::multiset<const Link *>{cc},
        "[DeleteNode] Adjacency of the nodes not adjacent to the deleted node should not be modified"
    );
    assertTrue(
        collectElements(G.findConnectingLinks("A", "B")) == std::multiset<const Link *>{ab}
            && collectElements(G.findConnectingLinks("B", "C")) == std::multiset<const Link *>{bc}
            && collectElements(G.findConnectingLinks("C", "C")) == std::multiset<const Link *>{cc},
        "[DeleteNode] Connecting links between the remaining nodes should not be modified"
    );

    // Unknown ID, or node already deleted: no-op.
    G.DeleteNode("Z");
    G.DeleteNode("E");
    G.checkInternalConsistency();
    assertTrue(
        G.nodeCount() == 4 && G.linkCount() == 3,
        "[DeleteNode] Deleting an unknown node should not modify the graph"
    );

    // Isolated node (i.e. without any adjacent link to remove).
    G.DeleteNode("D");
    G.checkInternalConsistency();
    assertTrue(G.nodeCount() == 3 && !G.hasNode("D"), "[DeleteNode] Isolated node 'D' should be removed");
    assertTrue(G.linkCount() == 3, "[DeleteNode] Deleting an isolated node should not remove any link");

    // A node with the same ID can be added again, and linked.
    G.AddNode("E", 5, 5);
    G.AddLink("B_E_1", "B", "E", 60, {{"CAR", {{"time", 6}}}}, "CarLayer");
    G.checkInternalConsistency();
    assertTrue(
        G.node("E")->x() == 5
            && collectElements(G.node("E")->inLinks()) == std::multiset<const Link *>{G.link("B_E_1")}
            && collectElements(b->outLinks()) == std::multiset<const Link *>{bc, G.link("B_E_1")}
            && collectElements(G.findConnectingLinks("B", "E")) == std::multiset<const Link *>{G.link("B_E_1")},
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
    G.checkInternalConsistency();
    assertTrue(G.node("A") == a && G.link("A_B") == ab, "[PointerStability] Insertions should not relocate elements");
    assertTrue(
        ab->up() == a && collectElements(G.findConnectingLinks("A", "B")) == std::multiset<const Link *>{ab},
        "[PointerStability] Relationships should survive insertions"
    );

    // Move construction/assignment transfer the elements without relocating them.
    OrientedGraph moved(std::move(G));
    moved.checkInternalConsistency();
    assertTrue(moved.node("A") == a && moved.link("A_B") == ab, "[PointerStability] Move should not relocate elements");
    OrientedGraph assigned;
    assigned = std::move(moved);
    assigned.checkInternalConsistency();
    assertTrue(
        assigned.node("A") == a && assigned.link("A_B") == ab,
        "[PointerStability] Move-assignment should not relocate elements"
    );

    // Copies are deep, with relationships pointing to the elements of the copy.
    OrientedGraph copy(assigned);
    copy.checkInternalConsistency();
    const Node *copyA = copy.node("A");
    const Link *copyAB = copy.link("A_B");
    assertTrue(copyA != a && copyAB != ab, "[PointerStability] Copy should create new elements");
    assertTrue(
        copyAB->up() == copyA && copyAB->down() == copy.node("B")
            && collectElements(copy.findConnectingLinks("A", "B")) == std::multiset<const Link *>{copyAB},
        "[PointerStability] Relationships in the copy should point to the elements of the copy"
    );
    OrientedGraph copyAssigned = makeGraph();
    copyAssigned = assigned;
    copyAssigned.checkInternalConsistency();
    assertTrue(
        copyAssigned.nodeCount() == assigned.nodeCount() && copyAssigned.linkCount() == assigned.linkCount(),
        "[PointerStability] Copy-assignment should replace the content"
    );
    assertTrue(
        copyAssigned.link("A_B")->up() == copyAssigned.node("A"),
        "[PointerStability] Relationships in the copy-assigned graph should point to its own elements"
    );
}


static void test_CopyConstructor() {

    OrientedGraph G = makeGraph();

    // Copy is deep, thus both instances are expected to be independent.
    OrientedGraph copyG(G);
    copyG.checkInternalConsistency();
    G.link("A_B")->setCosts({{"CAR", {{"time", 42}}}});

    assertTrue(
        G.link("A_B")->costs().at("CAR").at("time") != copyG.link("A_B")->costs().at("CAR").at("time"),
        "[CopyConstructor] Cost of original and copy should be different on link 'A_B'"
    );
    assertTrue(
        G.link("B_C")->costs().at("BUS").at("time") == copyG.link("B_C")->costs().at("BUS").at("time"),
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
    copyG.checkInternalConsistency();
    G.link("A_B")->setCosts({{"CAR", {{"time", 42}}}});

    assertTrue(!copyG.hasNode("Z"), "[CopyAssignmentOperator] Pre-existing node 'Z' should be erased by the copy");
    assertTrue(
        G.link("A_B")->costs().at("CAR").at("time") != copyG.link("A_B")->costs().at("CAR").at("time"),
        "[CopyAssignmentOperator] Cost of original and copy should be different on link 'A_B'"
    );
    assertTrue(
        G.link("B_C")->costs().at("BUS").at("time") == copyG.link("B_C")->costs().at("BUS").at("time"),
        "[CopyAssignmentOperator] Cost of original and copy should be the same on link 'B_C'"
    );
    assertTrue(G.link("B_C") != copyG.link("B_C"), "[CopyAssignmentOperator] Links must be different objects");
}


static void test_GetLinksWithoutCost() {

    const OrientedGraph G = makeGraph();
    const std::unordered_map<std::string, std::string> labelToCostFamily{
        {"CarLayer", "CAR"}, {"BusLayer", "BUS"}, {"TransitLayer", "WALK"}
    };

    std::unordered_set<std::string> result = G.GetLinksWithoutCost("time", labelToCostFamily);
    assertTrue(result.empty(), "[GetLinksWithoutCost] All links should have a cost for the given metric");

    result = G.GetLinksWithoutCost("distance", labelToCostFamily);
    assertTrue(
        result == std::unordered_set<std::string>{"A_B", "B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"},
        "[GetLinksWithoutCost] No link should have a cost for an unknown metric"
    );

    // Cost family defined on the bus links only.
    result = G.GetLinksWithoutCost("time", {{"CarLayer", "BUS"}, {"BusLayer", "BUS"}, {"TransitLayer", "BUS"}});
    assertTrue(
        result == std::unordered_set<std::string>{"A_B", "B_E_1", "B_E_3", "C_C", "E_B"},
        "[GetLinksWithoutCost] Links without the given cost family should be returned"
    );
}


static void test_Exits() {

    OrientedGraph G;
    G.AddNode("A", 0, 0);

    std::unordered_map<std::string, std::unordered_set<std::string>> excludeMovements;
    excludeMovements["A"] = {"C"};
    G.AddNode("B", 2, 5, "", excludeMovements);

    G.AddNode("C", 12., 43.);

    G.AddNode("D", 435, 345);
    G.AddLink("A_B", "A", "B", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.AddLink("B_C", "B", "C", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.AddLink("B_D", "B", "D", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");

    std::vector<const Link*> exits = G.node("B")->getExits("A");

    assertTrue(exits.size() == 1, "[Exits] getExits() should return one link");
    assertTrue(exits[0]->down()->id() == "D", "[Exits] Exit link should go to 'D'");

    // Movement restrictions apply alike to all the parallel links going to the same downstream node.
    G.AddLink("B_C_2", "B", "C", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.AddLink("B_D_2", "B", "D", 12, {{"CAR", {{"time", 12}}}}, "CarLayer");
    G.checkInternalConsistency();
    exits = G.node("B")->getExits("A");
    assertTrue(
        collectIds(exits) == std::multiset<std::string>{"B_D", "B_D_2"},
        "[Exits] getExits() should return all the parallel links to 'D', and none of those to 'C'"
    );
}


int testGraph(int, char**) {
    test_EmptyGraph();
    test_NodeAccessors();
    test_LinkAccessors();
    test_NodeRanges();
    test_LinkRanges();
    test_AdjacentLinks();
    test_ConnectingLinks();
    test_AddNode();
    test_AddLink();
    test_DeleteLink();
    test_DeleteParallelLink();
    test_DeleteNode();
    test_PointerStability();
    test_CopyConstructor();
    test_CopyAssignmentOperator();
    test_GetLinksWithoutCost();
    test_Exits();
    return 0;
}
