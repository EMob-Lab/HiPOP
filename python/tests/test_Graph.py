# Same test scenarios as in cpp/tests/testGraph.cpp, except for the parts
# that have no equivalent between the C++ and Python APIs.
# WARNING: any modification in this file should be reflected in the C++ test as well.

import copy
import re
from collections.abc import Callable

import pytest

from hipop.graph import AdjacentLinkCollection, OrientedGraph


# Graph used by the tests (figures correspond to link lengths):
#
#        10        30, 40, 99 (3 parallel links)
#   A --------> B ============> E
#               | <------------
#            20 |       25            x D (isolated)
#               v
#               C (*) Self-loop on C
def _make_graph() -> OrientedGraph:

    G = OrientedGraph()

    G.add_node("A", 0, 0)
    G.add_node("B", 1, 0)
    G.add_node("C", 1, 2)
    G.add_node("D", 8, 1)
    G.add_node("E", 3, 0)

    G.add_link("A_B", "A", "B", 10, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("B_C", "B", "C", 20, {"BUS": {"time": 2}}, "BusLayer")
    G.add_link("C_C", "C", "C", 0, {"WALK": {"time": 0.1}}, "TransitLayer")
    G.add_link("B_E_1", "B", "E", 30, {"CAR": {"time": 3}}, "CarLayer")
    G.add_link("B_E_2", "B", "E", 40, {"BUS": {"time": 4}}, "BusLayer")
    G.add_link("B_E_3", "B", "E", 99, {"WALK": {"time": 9}}, "TransitLayer")
    G.add_link("E_B", "E", "B", 25, {"CAR": {"time": 2.5}}, "CarLayer")

    return G


def _missing_key_regex(key: str) -> str:
    """Regex matching a KeyError message that mentions `key` as a whole word,
    whatever the quoting around it."""
    return rf"(?<!\w){re.escape(key)}(?!\w)"


def test_empty_graph() -> None:

    G = OrientedGraph()

    assert len(G.nodes) == 0, "Node view should be empty"
    assert len(G.links) == 0, "Link view should be empty"
    assert "A" not in G.nodes, "`in G.nodes` should be false"
    assert "A_B" not in G.links, "`in G.links` should be false"
    assert G.nodes.get("A") is None, "G.nodes.get() should return None"
    assert G.links.get("A_B") is None, "G.links.get() should return None"
    with pytest.raises(KeyError, match=_missing_key_regex("A")):
        G.nodes["A"]
    with pytest.raises(KeyError, match=_missing_key_regex("A_B")):
        G.links["A_B"]

    assert list(G.nodes) == [], "Node view should be empty"
    assert list(G.links) == [], "Link view should be empty"


def test_node_accessors() -> None:

    G = _make_graph()

    assert len(G.nodes) == 5, "Node view should have 5 elements"
    assert "A" in G.nodes and "D" in G.nodes, "`in G.nodes` should be true for existing nodes"
    assert "Z" not in G.nodes, "`in G.nodes` should be false for unknown nodes"

    # G.nodes[...]
    a = G.nodes["A"]
    assert a.id == "A" and a.position == [0, 0], "G.nodes[...] should return node 'A'"
    assert G.nodes["A"] is a, "G.nodes[...] should always return the same node"
    with pytest.raises(KeyError, match=_missing_key_regex("Z")):
        G.nodes["Z"]

    # G.nodes.get()
    assert G.nodes.get("A") is a, "G.nodes.get() should return node 'A'"
    assert G.nodes.get("Z") is None, "G.nodes.get() should return None for unknown nodes"

    # Accessors must stay consistent after a deletion.
    G.delete_node("B")
    assert len(G.nodes) == 4, "Node view should have 4 elements after deletion"
    assert "B" not in G.nodes, "`in G.nodes` should be false after deletion"
    assert G.nodes.get("B") is None, "G.nodes.get() should return None after deletion"
    with pytest.raises(KeyError, match=_missing_key_regex("B")):
        G.nodes["B"]
    assert sorted(G.nodes) == ["A", "C", "D", "E"], "View should not contain deleted node"


def test_link_accessors() -> None:

    G = _make_graph()

    assert len(G.links) == 7, "Link view should have 7 elements"
    assert "A_B" in G.links and "B_C" in G.links, "`in G.links` should be true for existing links"
    assert "Z" not in G.links, "`in G.links` should be false for unknown links"

    # G.links[...]
    ab = G.links["A_B"]
    assert ab.id == "A_B" and ab.label == "CarLayer" and ab.length == 10, "G.links[...] should return link 'A_B'"
    assert ab.upstream == "A" and ab.downstream == "B", "Link 'A_B' should go from 'A' to 'B'"
    assert G.links["A_B"] is ab, "G.links[...] should always return the same link"
    with pytest.raises(KeyError, match=_missing_key_regex("Z")):
        G.links["Z"]

    # G.links.get()
    assert G.links.get("A_B") is ab, "G.links.get() should return link 'A_B'"
    assert G.links.get("Z") is None, "G.links.get() should return None for unknown links"

    # Modification through the accessors.
    G.links["A_B"].update_costs({"CAR": {"time": 42}})
    assert G.links["A_B"].costs["CAR"]["time"] == 42, "Modification through G.links[...] should be visible"
    link = G.links.get("B_C")
    assert link is not None
    link.update_costs({"BUS": {"time": 43}})
    assert G.links["B_C"].costs["BUS"]["time"] == 43, "Modification through G.links.get() should be visible"

    # Accessors must stay consistent after a deletion.
    G.delete_link("A_B")
    assert len(G.links) == 6, "Link view should have 6 elements after deletion"
    assert "A_B" not in G.links, "`in G.links` should be false after deletion"
    assert G.links.get("A_B") is None, "G.links.get() should return None after deletion"
    with pytest.raises(KeyError, match=_missing_key_regex("A_B")):
        G.links["A_B"]
    assert sorted(G.links) == ["B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"], "View should not contain deleted link"


def test_node_ranges() -> None:

    G = _make_graph()

    expected = ["A", "B", "C", "D", "E"]
    assert sorted(G.nodes) == expected, "View should enumerate each node once"
    assert sorted(G.nodes.keys()) == expected, "Key view should enumerate each node once"
    assert sorted(node.id for node in G.nodes.values()) == expected, "Value view should enumerate each node once"
    assert sorted(node_id for node_id, _ in G.nodes.items()) == expected, "Item view should enumerate each node once"

    for node in G.nodes.values():
        assert node is G.nodes[node.id], "View should enumerate the nodes of the graph"
    for node_id, node in G.nodes.items():
        assert node.id == node_id, "Item view should associate each node with its ID"

    # Python-specific: dict-like features.
    assert len(G.nodes.keys()) == len(G.nodes.values()) == len(G.nodes.items()) == 5, "Views should have 5 elements"
    assert "A" in G.nodes.keys() and "Z" not in G.nodes.keys(), "`in G.nodes.keys()` should work"  # noqa: SIM118
    assert dict(G.nodes) == {node.id: node for node in G.nodes.values()}, "Node view should be convertible to a dict"


def test_link_ranges() -> None:

    G = _make_graph()

    expected = ["A_B", "B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"]
    assert sorted(G.links) == expected, "View should enumerate each link once"
    assert sorted(G.links.keys()) == expected, "Key view should enumerate each link once"
    assert sorted(link.id for link in G.links.values()) == expected, "Value view should enumerate each link once"
    assert sorted(link_id for link_id, _ in G.links.items()) == expected, "Item view should enumerate each link once"

    for link in G.links.values():
        assert link is G.links[link.id], "View should enumerate the links of the graph"
    for link_id, link in G.links.items():
        assert link.id == link_id, "Item view should associate each link with its ID"

    # Modification through the view.
    for link in G.links.values():
        link.update_costs({"CAR": {"time": 100}})
    for link in G.links.values():
        assert link.costs["CAR"]["time"] == 100, "Modification through the view should be visible"

    # Python-specific: dict-like features.
    assert len(G.links.keys()) == len(G.links.values()) == len(G.links.items()) == 7, "Views should have 7 elements"
    assert "A_B" in G.links.keys() and "Z" not in G.links.keys(), "`in G.links.keys()` should work"  # noqa: SIM118
    assert dict(G.links) == {link.id: link for link in G.links.values()}, "Link view should be convertible to a dict"


def test_adjacent_links() -> None:

    G = _make_graph()
    a = G.nodes["A"]
    b = G.nodes["B"]
    c = G.nodes["C"]
    d = G.nodes["D"]
    e = G.nodes["E"]

    # Python-specific
    assert isinstance(b.in_links, AdjacentLinkCollection) and isinstance(b.out_links, AdjacentLinkCollection), (
        "in_links/out_links should be views"
    )
    assert all(link is G.links[link.id] for link in b.in_links), "in_links should yield the Link objects of the graph"
    assert all(link is G.links[link.id] for link in b.out_links), "out_links should yield the Link objects of the graph"

    assert len(a.out_links) == 1 and a.out_links.ids() == {"A_B"}, (
        "out_links should enumerate the outgoing links"
    )
    assert len(b.in_links) == 2 and b.in_links.ids() == {"A_B", "E_B"}, (
        "in_links should enumerate the incoming links"
    )
    assert len(b.out_links) == 4 and b.out_links.ids() == {"B_C", "B_E_1", "B_E_2", "B_E_3"}, (
        "out_links should enumerate the parallel outgoing links"
    )
    assert len(e.in_links) == 3 and e.in_links.ids() == {"B_E_1", "B_E_2", "B_E_3"}, (
        "in_links should enumerate the parallel incoming links"
    )
    assert len(d.in_links) == 0 and len(d.out_links) == 0, "Isolated node should have no adjacent link"

    # A self-loop is both an incoming and an outgoing link.
    assert c.in_links.ids() == {"B_C", "C_C"} and c.out_links.ids() == {"C_C"}, (
        "Self-loop should be both an incoming and an outgoing link"
    )



def test_connecting_links() -> None:

    G = _make_graph()

    # Python-specific
    assert isinstance(G.find_connecting_links("B", "E"), set), "find_connecting_links() should return a plain set"
    assert all(link is G.links[link.id] for link in G.find_connecting_links("B", "E")), (
        "find_connecting_links() should return the Link objects of the graph"
    )

    assert (
        G.has_connecting_link("B", "E") and G.count_connecting_links("B", "E") == 3
        and {link.id for link in G.find_connecting_links("B", "E")} == {"B_E_1", "B_E_2", "B_E_3"}
        and G.find_connecting_link_ids("B", "E") == {"B_E_1", "B_E_2", "B_E_3"}
    ), "Parallel links from 'B' to 'E' should be found"
    assert (
        G.has_connecting_link("A", "B") and G.count_connecting_links("A", "B") == 1
        and {link.id for link in G.find_connecting_links("A", "B")} == {"A_B"}
        and G.find_connecting_link_ids("A", "B") == {"A_B"}
    ), "Link from 'A' to 'B' should be found"
    assert (
        not G.has_connecting_link("B", "A") and G.count_connecting_links("B", "A") == 0
        and G.find_connecting_links("B", "A") == set()
        and G.find_connecting_link_ids("B", "A") == set()
    ), "No link should be found from 'B' to 'A' (direction matters)"
    assert (
        G.has_connecting_link("E", "B") and G.count_connecting_links("E", "B") == 1
        and {link.id for link in G.find_connecting_links("E", "B")} == {"E_B"}
        and G.find_connecting_link_ids("E", "B") == {"E_B"}
    ), "Link from 'E' to 'B' should be found (direction matters)"
    assert (
        G.has_connecting_link("C", "C") and G.count_connecting_links("C", "C") == 1
        and {link.id for link in G.find_connecting_links("C", "C")} == {"C_C"}
        and G.find_connecting_link_ids("C", "C") == {"C_C"}
    ), "Self-loop on 'C' should be found"
    assert (
        not G.has_connecting_link("A", "C") and G.count_connecting_links("A", "C") == 0
        and G.find_connecting_links("A", "C") == set()
        and G.find_connecting_link_ids("A", "C") == set()
    ), "No link should be found between non-adjacent nodes"
    assert (
        not G.has_connecting_link("A", "A") and G.count_connecting_links("A", "A") == 0
        and G.find_connecting_links("A", "A") == set()
        and G.find_connecting_link_ids("A", "A") == set()
    ), "No link should be found from 'A' to 'A'"

    # Unknown node IDs.
    with pytest.raises(ValueError):
        G.has_connecting_link("Z", "A")
    with pytest.raises(ValueError):
        G.has_connecting_link("A", "Z")
    with pytest.raises(ValueError):
        G.count_connecting_links("Z", "A")
    with pytest.raises(ValueError):
        G.count_connecting_links("A", "Z")
    with pytest.raises(ValueError):
        G.find_connecting_links("Z", "A")
    with pytest.raises(ValueError):
        G.find_connecting_links("A", "Z")
    with pytest.raises(ValueError):
        G.find_connecting_link_ids("Z", "A")
    with pytest.raises(ValueError):
        G.find_connecting_link_ids("A", "Z")


def test_add_node() -> None:

    G = _make_graph()
    a = G.nodes["A"]

    G.add_node("Z", 4, 4, "NewNode")
    z = G.nodes["Z"]
    assert len(G.nodes) == 6, "Node view should have 6 elements after insertion"
    assert z.id == "Z" and z.position == [4, 4] and z.label == "NewNode", "G.nodes[...] should return the inserted node"

    # Duplicated ID.
    with pytest.raises(ValueError):
        G.add_node("A", 5, 5, "Duplicate")
    assert len(G.nodes) == 6, "Node view should be unchanged after a failed insertion"
    assert G.nodes["A"] is a, "Existing node should not be replaced by a failed insertion"
    assert a.position == [0, 0] and a.label == "", "Existing node should not be modified by a failed insertion"
    assert a.out_links.ids() == {"A_B"} and len(a.in_links) == 0, (
        "Links of the existing node should not be modified by a failed insertion"
    )


def test_add_link() -> None:

    G = _make_graph()
    ab = G.links["A_B"]

    G.add_link("C_D", "C", "D", 30, {"CAR": {"time": 3}}, "CarLayer")
    cd = G.links["C_D"]
    assert len(G.links) == 8, "Link view should have 8 elements after insertion"
    assert cd.id == "C_D" and cd.upstream == "C" and cd.downstream == "D" and cd.length == 30, (
        "G.links[...] should return the inserted link"
    )
    assert G.nodes["C"].out_links.ids() == {"C_C", "C_D"} and G.nodes["D"].in_links.ids() == {"C_D"}, (
        "Inserted link should be registered in the adjacency of its nodes"
    )
    assert {link.id for link in G.find_connecting_links("C", "D")} == {"C_D"}, (
        "Inserted link should be found as a connecting link between its nodes"
    )

    # Duplicated ID.
    with pytest.raises(ValueError):
        G.add_link("A_B", "D", "A", 40, {"CAR": {"time": 4}}, "Duplicate")
    assert len(G.links) == 8, "Link view should be unchanged after a failed insertion"
    assert G.links["A_B"] is ab, "Existing link should not be replaced by a failed insertion"
    assert ab.upstream == "A" and ab.downstream == "B" and ab.length == 10 and ab.label == "CarLayer", (
        "Existing link should not be modified by a failed insertion"
    )
    assert len(G.nodes["D"].out_links) == 0 and len(G.nodes["A"].in_links) == 0, (
        "Failed insertion should not modify the adjacency of the nodes"
    )
    assert (
        not G.has_connecting_link("D", "A")
        and {link.id for link in G.find_connecting_links("A", "B")} == {"A_B"}
    ), "Failed insertion should not modify the connecting links"

    # Unknown upstream/downstream node.
    with pytest.raises(ValueError):
        G.add_link("Z_A", "Z", "A", 50, {"CAR": {"time": 5}}, "CarLayer")
    with pytest.raises(ValueError):
        G.add_link("A_Z", "A", "Z", 50, {"CAR": {"time": 5}}, "CarLayer")
    assert len(G.links) == 8, "Link view should be unchanged after a failed insertion"
    assert "Z_A" not in G.links and "A_Z" not in G.links, "Failed insertion should not add any link"
    assert "Z" not in G.nodes, "Failed insertion should not add any node"
    assert len(G.nodes["A"].out_links) == 1 and len(G.nodes["A"].in_links) == 0, (
        "Failed insertion should not modify the adjacency of the nodes"
    )


def test_delete_link() -> None:

    G = _make_graph()
    a = G.nodes["A"]
    b = G.nodes["B"]
    bc = G.links["B_C"]

    G.delete_link("A_B")
    assert len(G.links) == 6 and "A_B" not in G.links, "Link 'A_B' should be removed"
    assert len(G.nodes) == 5, "Nodes should not be removed"
    assert G.nodes["A"] is a and G.nodes["B"] is b and G.links["B_C"] is bc, (
        "Remaining elements should not be replaced"
    )
    assert len(a.out_links) == 0 and b.in_links.ids() == {"E_B"}, (
        "Deleted link should be unregistered from the adjacency of its nodes"
    )
    assert b.out_links.ids() == {"B_C", "B_E_1", "B_E_2", "B_E_3"}, (
        "Adjacency of the other links should not be modified"
    )
    assert G.nodes["C"].in_links.ids() == {"B_C", "C_C"}, (
        "Adjacency of the other links should not be modified"
    )
    assert (
        G.find_connecting_links("A", "B") == set()
        and {link.id for link in G.find_connecting_links("B", "E")} == {"B_E_1", "B_E_2", "B_E_3"}
    ), "Deleted link should not be found as a connecting link anymore, unlike the other links"

    # Unknown ID, or link already deleted: no-op.
    G.delete_link("Z")
    G.delete_link("A_B")
    assert len(G.links) == 6 and len(G.nodes) == 5, "Deleting an unknown link should not modify the graph"

    # A link with the same ID and endpoints can be added again.
    G.add_link("A_B", "A", "B", 15, {"CAR": {"time": 5}}, "CarLayer")
    assert G.links["A_B"].length == 15 and a.out_links.ids() == {"A_B"}, "Deleted link should be re-insertable"
    assert b.in_links.ids() == {"A_B", "E_B"}, "Deleted link should be re-insertable"
    assert {link.id for link in G.find_connecting_links("A", "B")} == {"A_B"}, (
        "Deleted link should be re-insertable"
    )


def test_delete_parallel_link() -> None:

    G = _make_graph()
    b = G.nodes["B"]
    e = G.nodes["E"]
    be1 = G.links["B_E_1"]
    be3 = G.links["B_E_3"]

    # Removal of one of the parallel links from B to E (the one in the middle of the insertion order).
    G.delete_link("B_E_2")
    assert len(G.links) == 6 and "B_E_2" not in G.links, "Link 'B_E_2' should be removed"
    assert G.links["B_E_1"] is be1 and G.links["B_E_3"] is be3, "Other parallel links should not be replaced"
    assert b.out_links.ids() == {"B_C", "B_E_1", "B_E_3"} and e.in_links.ids() == {"B_E_1", "B_E_3"}, (
        "Other parallel links should remain in the adjacency of their nodes"
    )
    assert (
        G.count_connecting_links("B", "E") == 2
        and {link.id for link in G.find_connecting_links("B", "E")} == {"B_E_1", "B_E_3"}
    ), "Other parallel links should still be found as connecting links"
    assert {link.id for link in G.find_connecting_links("E", "B")} == {"E_B"}, (
        "Link in the opposite direction should not be affected"
    )


def test_delete_node() -> None:

    G = _make_graph()
    a = G.nodes["A"]
    b = G.nodes["B"]
    c = G.nodes["C"]
    d = G.nodes["D"]
    ab = G.links["A_B"]
    bc = G.links["B_C"]
    cc = G.links["C_C"]

    G.delete_node("E")
    assert len(G.nodes) == 4 and "E" not in G.nodes, "Node 'E' should be removed"
    assert sorted(G.links) == ["A_B", "B_C", "C_C"], "All the links from or to node 'E' should be removed"
    assert G.nodes["A"] is a and G.nodes["B"] is b and G.nodes["C"] is c and G.nodes["D"] is d, (
        "Remaining nodes should not be replaced"
    )
    assert G.links["A_B"] is ab and G.links["B_C"] is bc and G.links["C_C"] is cc, (
        "Remaining links should not be replaced"
    )
    assert b.in_links.ids() == {"A_B"} and b.out_links.ids() == {"B_C"}, (
        "Deleted links should be unregistered from the adjacency of the remaining nodes"
    )
    assert a.out_links.ids() == {"A_B"} and c.in_links.ids() == {"B_C", "C_C"} \
        and c.out_links.ids() == {"C_C"}, (
        "Adjacency of the nodes not adjacent to the deleted node should not be modified"
    )
    assert (
        {link.id for link in G.find_connecting_links("A", "B")} == {"A_B"}
        and {link.id for link in G.find_connecting_links("B", "C")} == {"B_C"}
        and {link.id for link in G.find_connecting_links("C", "C")} == {"C_C"}
    ), "Connecting links between the remaining nodes should not be modified"

    # Unknown ID, or node already deleted: no-op.
    G.delete_node("Z")
    G.delete_node("E")
    assert len(G.nodes) == 4 and len(G.links) == 3, "Deleting an unknown node should not modify the graph"

    # Isolated node (i.e. without any adjacent link to remove).
    G.delete_node("D")
    assert len(G.nodes) == 3 and "D" not in G.nodes, "Isolated node 'D' should be removed"
    assert len(G.links) == 3, "Deleting an isolated node should not remove any link"

    # A node with the same ID can be added again, and linked.
    G.add_node("E", 5, 5)
    G.add_link("B_E_1", "B", "E", 60, {"CAR": {"time": 6}}, "CarLayer")
    assert G.nodes["E"].position == [5, 5] and G.nodes["E"].in_links.ids() == {"B_E_1"}, (
        "Deleted node should be re-insertable"
    )
    assert b.out_links.ids() == {"B_C", "B_E_1"}, "Deleted node should be re-insertable"
    assert {link.id for link in G.find_connecting_links("B", "E")} == {"B_E_1"}, (
        "Deleted node should be re-insertable"
    )


@pytest.mark.parametrize("copy_func", [
    OrientedGraph.copy,
    copy.copy,
    copy.deepcopy,
], ids=["copy_method", "copy.copy", "copy.deepcopy"])
def test_copy(copy_func: Callable[[OrientedGraph], OrientedGraph]) -> None:

    G = _make_graph()
    copyG = copy_func(G)
    G.links["A_B"].update_costs({"CAR": {"time": 42}})

    # Since the graph owns its nodes and links, all copy methods are expected to be deep copies,
    # thus both instances are expected to be independent.
    assert G.links["A_B"].costs != copyG.links["A_B"].costs, \
        "Cost of original and copy should be different on link 'A_B'"
    assert G.links["B_C"].costs == copyG.links["B_C"].costs, \
        "Cost of original and copy should be the same on link 'B_C'"
    assert G.links["B_C"] is not copyG.links["B_C"], "Links must be different objects"


def test_get_links_without_cost() -> None:

    G = _make_graph()
    label_to_cost_family = {"CarLayer": "CAR", "BusLayer": "BUS", "TransitLayer": "WALK"}

    result = G.get_links_without_cost("time", label_to_cost_family)
    assert result == set(), "All links should have a cost for the given metric"

    result = G.get_links_without_cost("distance", label_to_cost_family)
    assert result == {"A_B", "B_C", "B_E_1", "B_E_2", "B_E_3", "C_C", "E_B"}, (
        "No link should have a cost for an unknown metric"
    )

    # Cost family defined on the bus links only.
    result = G.get_links_without_cost("time", {"CarLayer": "BUS", "BusLayer": "BUS", "TransitLayer": "BUS"})
    assert result == {"A_B", "B_E_1", "B_E_3", "C_C", "E_B"}, "Links without the given cost family should be returned"


def test_exits() -> None:

    G = OrientedGraph()
    G.add_node("A", 0, 0)

    exclude_movements = {"A": {"C"}}
    G.add_node("B", 2, 5, exclude_movements=exclude_movements)

    G.add_node("C", 12.0, 43.0)

    G.add_node("D", 435, 345)
    G.add_link("A_B", "A", "B", 12, {"CAR": {"time": 12}}, "CarLayer")
    G.add_link("B_C", "B", "C", 12, {"CAR": {"time": 12}}, "CarLayer")
    G.add_link("B_D", "B", "D", 12, {"CAR": {"time": 12}}, "CarLayer")

    exits = G.nodes["B"].get_exits("A")

    assert len(exits) == 1, "get_exits() should return one link"
    assert exits[0].downstream == "D", "Exit link should go to 'D'"

    # Movement restrictions apply alike to all the parallel links going to the same downstream node.
    G.add_link("B_C_2", "B", "C", 12, {"CAR": {"time": 12}}, "CarLayer")
    G.add_link("B_D_2", "B", "D", 12, {"CAR": {"time": 12}}, "CarLayer")
    exits = G.nodes["B"].get_exits("A")
    assert {link.id for link in exits} == {"B_D", "B_D_2"}, (
        "get_exits() should return all the parallel links to 'D', and none of those to 'C'"
    )
