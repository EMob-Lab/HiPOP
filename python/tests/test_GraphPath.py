import copy

import pytest

from hipop.graph import OrientedGraph, OrientedGraphPath, make_empty_path, make_path


# Small graph used in all the test cases:
#
# ```
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
    assert path.node(0).id == "A"
    assert path.node_id(0) == "A"
    assert [node.id for node in path.nodes] == ["A"]
    assert path.node_ids == ["A"]
    assert path.links == []
    assert path.link_ids == []

    with pytest.raises(IndexError):
        path.node(1)
    with pytest.raises(IndexError):
        path.link(0)
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
    assert path.node(1).id == "B"
    assert path.node_id(1) == "B"
    assert path.link(1).id == "B_C"
    assert path.link_id(1) == "B_C"
    assert [node.id for node in path.nodes] == ["A", "B", "C", "D"]
    assert path.node_ids == ["A", "B", "C", "D"]
    assert [link.id for link in path.links] == ["A_B", "B_C", "C_D"]
    assert path.link_ids == ["A_B", "B_C", "C_D"]

    with pytest.raises(IndexError):
        path.node(4)
    with pytest.raises(IndexError):
        path.link(3)

    with pytest.raises(ValueError):
        make_path(G, [])
    with pytest.raises(ValueError):
        make_path(G, ["A_B", "C_D"])
    with pytest.raises(ValueError, match=r"\[I'm not a link ID\]"):
        make_path(G, ["A_B", "I'm not a link ID"])


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
