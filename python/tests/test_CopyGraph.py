from hipop.graph import OrientedGraph, copy_graph


def _make_initial_graph() -> OrientedGraph:

    G = OrientedGraph()

    G.add_node("0", 0, 0, "")
    G.add_node("1", 1, 0, "")
    G.add_node("2", 1, 1, "")
    G.add_node("3", 0, 1, "")

    G.add_link("0_1", "0", "1", 1, {"PersonalVehicle": {"time": 12}}, "CAR")
    G.add_link("1_2", "1", "2", 1, {"PersonalVehicle": {"time": 13}}, "CAR")
    G.add_link("0_3", "0", "3", 1, {"PersonalVehicle": {"time": 12}}, "CAR")
    G.add_link("3_2", "3", "2", 1, {"PersonalVehicle": {"time": 12}}, "CAR")

    return G


# Same scenario as testCopyGraph.cpp, adapted to Python: `copy_graph` is expected
# to be a deep copy, so both instances must be independent afterwards.
def test_copy_graph() -> None:

    G = _make_initial_graph()
    copyG = copy_graph(G)

    G.links["0_1"].update_costs({"PersonalVehicle": {"time": 42}})

    assert G.links["0_1"].costs != copyG.links["0_1"].costs, \
        "Cost of original and copy should be different on link '0_1'"
    assert G.links["0_3"].costs == copyG.links["0_3"].costs, \
        "Cost of original and copy should be the same on link '0_3'"
    assert G.links["0_3"] is not copyG.links["0_3"], \
        "Links must be different objects"


# In Python, assigning a graph to another variable does not copy anything:
# both variables are just names bound to the same instance.
def test_variable_assignment_is_not_a_copy() -> None:

    G = _make_initial_graph()
    sameG = G

    assert sameG is G, "Both variables should point to the same object"

    G.links["0_1"].update_costs({"PersonalVehicle": {"time": 42}})

    assert G.links["0_1"] is sameG.links["0_1"], \
        "Links should be the same object"
    assert G.links["0_1"].costs == sameG.links["0_1"].costs, \
        "Cost of original and copy should be the same on link '0_1'"
