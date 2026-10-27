# Same test scenarios as in cpp/tests/testGraph.cpp, except for the parts
# that have no equivalent between the C++ and Python APIs.
# WARNING: any modification in this file should be reflected in the C++ test as well.

import re

import pytest

from hipop.graph import OrientedGraph


#        10          20
#   A --------> B --------> C      D (isolated)
def _make_graph() -> OrientedGraph:

    G = OrientedGraph()

    G.add_node("A", 0, 0, "")
    G.add_node("B", 1, 0, "")
    G.add_node("C", 2, 0, "")
    G.add_node("D", 3, 3, "")

    G.add_link("A_B", "A", "B", 10, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("B_C", "B", "C", 20, {"BUS": {"time": 2}}, "BusLayer")

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

    assert len(G.nodes) == 4, "Node view should have 4 elements"
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


def test_link_accessors() -> None:

    G = _make_graph()

    assert len(G.links) == 2, "Link view should have 2 elements"
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
    assert len(G.links) == 1, "Link view should have 1 element after deletion"
    assert "A_B" not in G.links, "`in G.links` should be false after deletion"
    assert G.links.get("A_B") is None, "G.links.get() should return None after deletion"
    with pytest.raises(KeyError, match=_missing_key_regex("A_B")):
        G.links["A_B"]
    assert list(G.links) == ["B_C"], "View should not contain deleted link"


def test_node_ranges() -> None:

    G = _make_graph()

    expected = ["A", "B", "C", "D"]
    assert sorted(G.nodes) == expected, "View should enumerate each node once"
    assert sorted(G.nodes.keys()) == expected, "Key view should enumerate each node once"
    assert sorted(node.id for node in G.nodes.values()) == expected, "Value view should enumerate each node once"
    assert sorted(node_id for node_id, _ in G.nodes.items()) == expected, "Item view should enumerate each node once"

    for node in G.nodes.values():
        assert node is G.nodes[node.id], "View should enumerate the nodes of the graph"
    for node_id, node in G.nodes.items():
        assert node.id == node_id, "Item view should associate each node with its ID"

    # Python-specific: dict-like features.
    assert len(G.nodes.keys()) == len(G.nodes.values()) == len(G.nodes.items()) == 4, "Views should have 4 elements"
    assert "A" in G.nodes.keys() and "Z" not in G.nodes.keys(), "`in G.nodes.keys()` should work"  # noqa: SIM118
    assert dict(G.nodes) == {node.id: node for node in G.nodes.values()}, "Node view should be convertible to a dict"


def test_link_ranges() -> None:

    G = _make_graph()

    expected = ["A_B", "B_C"]
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
    assert len(G.links.keys()) == len(G.links.values()) == len(G.links.items()) == 2, "Views should have 2 elements"
    assert "A_B" in G.links.keys() and "Z" not in G.links.keys(), "`in G.links.keys()` should work"  # noqa: SIM118
    assert dict(G.links) == {link.id: link for link in G.links.values()}, "Link view should be convertible to a dict"


def test_exits() -> None:

    G = OrientedGraph()
    G.add_node("A", 0, 0, "")

    exclude_movements = {"A": {"C"}}
    G.add_node("B", 2, 5, "", exclude_movements)

    G.add_node("C", 12.0, 43.0, "")

    G.add_node("D", 435, 345, "")
    G.add_link("A_B", "A", "B", 12, {"CAR": {"time": 12}}, "CarLayer")
    G.add_link("B_C", "B", "C", 12, {"CAR": {"time": 12}}, "CarLayer")
    G.add_link("B_D", "B", "D", 12, {"CAR": {"time": 12}}, "CarLayer")

    exits = G.nodes["B"].get_exits("A")

    assert len(exits) == 1, "get_exits() should return one link"
    assert exits[0].downstream == "D", "Exit link should go to 'D'"
