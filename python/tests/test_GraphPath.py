import copy

import pytest

from hipop.graph import OrientedGraph, OrientedGraphPath, make_empty_path, make_path


# Small graph used in all the test cases:
#
# ```
#   +-----------------------------------+
#   |                                   |
#   v                                   |
#   A --------> B --------> C --------> D
#   |                       ^
#   |                       |
#   +-----------------------+
# ```
#
# REMARK: here, the goal is to validate the bindings, not to cover
# all the algorithmic edge cases (these are covered in cpp/tests/testGraphPath.cpp).
def _make_graph() -> OrientedGraph:

    G = OrientedGraph()

    G.add_node("A", 0, 0, "")
    G.add_node("B", 1, 0, "")
    G.add_node("C", 2, 0, "")
    G.add_node("D", 3, 0, "")

    G.add_link("A_B", "A", "B", 1, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("B_C", "B", "C", 1, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("C_D", "C", "D", 1, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("A_C", "A", "C", 1, {"CAR": {"time": 1}}, "CarLayer")
    G.add_link("D_A", "D", "A", 1, {"CAR": {"time": 1}}, "CarLayer")

    return G


def test_empty_path() -> None:

    G = _make_graph()
    path = make_empty_path(G, "A")

    assert len(path) == 0
    assert path, "An empty path must be truthy"
    assert path.origin.id == "A"
    assert path.origin_id == "A"
    assert path.destination.id == "A"
    assert path.destination_id == "A"
    assert len(path.nodes) == 1
    assert path.nodes[0].id == "A"
    assert [node.id for node in path.nodes] == ["A"]
    assert path.nodes.ids() == ["A"]
    assert len(path.links) == 0
    assert not path.links, "The link view of an empty path must be falsy"
    assert list(path.links) == []
    assert path.links.ids() == []
    assert not path.has_loop()

    with pytest.raises(IndexError):
        path.nodes[1]
    with pytest.raises(IndexError):
        path.links[0]
    with pytest.raises(ValueError, match=r"\[I'm not a node ID\]"):
        make_empty_path(G, "I'm not a node ID")


def test_make_path() -> None:

    G = _make_graph()
    path = make_path(G, ["A_B", "B_C", "C_D"])

    assert len(path) == 3
    assert path.origin.id == "A"
    assert path.origin_id == "A"
    assert path.destination.id == "D"
    assert path.destination_id == "D"
    assert path.nodes[1].id == "B"
    assert path.links[1].id == "B_C"
    assert [node.id for node in path.nodes] == ["A", "B", "C", "D"]
    assert path.nodes.ids() == ["A", "B", "C", "D"]
    assert [link.id for link in path.links] == ["A_B", "B_C", "C_D"]
    assert path.links.ids() == ["A_B", "B_C", "C_D"]
    assert not path.has_loop()
    assert make_path(G, ["A_B", "B_C", "C_D", "D_A"]).has_loop()

    with pytest.raises(IndexError):
        path.nodes[4]
    with pytest.raises(IndexError):
        path.links[3]

    with pytest.raises(ValueError):
        make_path(G, [])
    with pytest.raises(ValueError):
        make_path(G, ["A_B", "C_D"])
    with pytest.raises(ValueError, match=r"\[I'm not a link ID\]"):
        make_path(G, ["A_B", "I'm not a link ID"])


def test_node_and_link_views() -> None:

    G = _make_graph()
    path = make_path(G, ["A_B", "B_C", "C_D"])

    for node in path.nodes:
        assert node is G.nodes[node.id], "View should enumerate the nodes of the graph"
    for link in path.links:
        assert link is G.links[link.id], "View should enumerate the links of the graph"

    # Python-specific: list-like features.
    assert len(path.nodes) == 4 and len(path.links) == 3
    assert path.nodes[-1].id == "D" and path.nodes[-4].id == "A", "Negative indexing should work"
    assert path.links[-1].id == "C_D" and path.links[-3].id == "A_B", "Negative indexing should work"
    with pytest.raises(IndexError):
        path.nodes[-5]
    with pytest.raises(IndexError):
        path.links[-4]
    assert [node.id for node in reversed(path.nodes)] == ["D", "C", "B", "A"], "`reversed()` should work"
    assert [link.id for link in reversed(path.links)] == ["C_D", "B_C", "A_B"], "`reversed()` should work"
    assert G.nodes["B"] in path.nodes and G.nodes["A"] in path.nodes, "`in path.nodes` should work"
    assert G.links["B_C"] in path.links and G.links["A_C"] not in path.links, "`in path.links` should work"
    assert [node.id for node in list(path.nodes)] == path.nodes.ids(), "Node view should be convertible to a list"
    assert [link.id for link in list(path.links)] == path.links.ids(), "Link view should be convertible to a list"

    # The views reflect the current state of the path.
    nodes = path.nodes
    links = path.links
    path.add(G.links["D_A"])
    assert len(nodes) == 5 and nodes[-1].id == "A", "Node view should reflect modifications of the path"
    assert len(links) == 4 and links[-1].id == "D_A", "Link view should reflect modifications of the path"

    # The views keep the path alive.
    links = make_path(G, ["A_B", "B_C"]).links
    assert [link.id for link in links] == ["A_B", "B_C"], "Link view should keep the path alive"


def test_constructors() -> None:

    G = _make_graph()

    assert OrientedGraphPath(G.nodes["A"]) == make_empty_path(G, "A")
    assert OrientedGraphPath(G.nodes["A"], [G.links["A_B"], G.links["B_C"]]) == make_path(G, ["A_B", "B_C"])

    with pytest.raises(ValueError):
        OrientedGraphPath(G.nodes["B"], [G.links["A_B"]])


def test_equality() -> None:

    G = _make_graph()

    assert make_path(G, ["A_B", "B_C"]) == make_path(G, ["A_B", "B_C"])
    assert make_path(G, ["A_B", "B_C"]) != make_path(G, ["A_C"])
    assert make_empty_path(G, "A") != make_empty_path(G, "B")

    # Support comparison with other object types
    assert make_path(G, ["A_B", "B_C"]) != "I'm not a path"


def test_hash() -> None:

    G = _make_graph()
    path1 = make_path(G, ["A_B", "B_C"])
    path2 = make_path(G, ["A_B", "B_C"])
    assert path1 == path2

    assert hash(path1) == hash(path2), "Equal paths must have the same hash value"
    assert len({path1, path2}) == 1, "Equal paths must be deduplicated in a set"


def test_add() -> None:

    G = _make_graph()

    path = make_empty_path(G, "A")
    path.add(G.links["A_B"])
    path.add(make_path(G, ["B_C", "C_D"]))
    assert path == make_path(G, ["A_B", "B_C", "C_D"])

    with pytest.raises(ValueError):
        path.add(G.links["A_B"])
    with pytest.raises(ValueError):
        path.add(make_path(G, ["A_B"]))
    assert path == make_path(G, ["A_B", "B_C", "C_D"]), "Failed insertions must leave the path unchanged"


def test_sub_path() -> None:

    G = _make_graph()
    path = make_path(G, ["A_B", "B_C", "C_D"])

    assert path.sub_path(1) == make_path(G, ["B_C", "C_D"])
    assert path.sub_path(1, 1) == make_path(G, ["B_C"])
    assert path.sub_path(3) == make_empty_path(G, "D")

    assert path.starts_with(make_path(G, ["A_B", "B_C"]))
    assert not path.starts_with(make_path(G, ["A_C"]))
    assert path.ends_with(make_path(G, ["C_D"]))
    assert not path.ends_with(make_path(G, ["B_C"]))


def test_copy() -> None:

    G = _make_graph()
    path = make_path(G, ["A_B"])

    for path_copy in (path.copy(), copy.copy(path), copy.deepcopy(path)):
        assert path_copy == path
        path_copy.add(G.links["B_C"])
        assert path_copy == make_path(G, ["A_B", "B_C"])
        assert path == make_path(G, ["A_B"]), "Modifying the copy must not affect the original path"


def test_repr() -> None:

    G = _make_graph()

    assert repr(make_path(G, ["A_B", "B_C"])) == "OrientedGraphPath(from=A links=[A_B B_C] to=C)"
    assert repr(make_empty_path(G, "A")) == "OrientedGraphPath(from=A links=[] to=A)"
