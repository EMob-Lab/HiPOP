from typing import TypedDict

# Re-export symbols from the C++ bindings
from hipop.cpp.graph import Link as Link
from hipop.cpp.graph import Node as Node
from hipop.cpp.graph import OrientedGraph as OrientedGraph
from hipop.cpp.graph import generate_manhattan as generate_manhattan
from hipop.cpp.graph import merge_oriented_graph as merge_oriented_graph
from hipop.cpp.graph import copy_graph as copy_graph


class NodeDict(TypedDict):
    ID: str
    X: float
    Y: float
    LABEL: str
    EXCLUDE_MOVEMENTS: dict[str, set[str]]


class LinkDict(TypedDict):
    ID: str
    UPSTREAM: str
    DOWNSTREAM: str
    LENGTH: float
    COSTS: dict[str, dict[str, float]]
    LABEL: str


class GraphDict(TypedDict):
    NODES: list[NodeDict]
    LINKS: list[LinkDict]


def node_to_dict(node: Node) -> NodeDict:
    return {
        "ID": node.id,
        "X": node.position[0],
        "Y": node.position[1],
        "LABEL": node.label,
        "EXCLUDE_MOVEMENTS": node.exclude_movements,
    }


def dict_to_node(G: OrientedGraph, d: NodeDict) -> None:
    G.add_node(d["ID"],
               d["X"],
               d["Y"],
               d["LABEL"],
               d["EXCLUDE_MOVEMENTS"])


def link_to_dict(link: Link) -> LinkDict:
    return {
        "ID": link.id,
        "UPSTREAM": link.upstream,
        "DOWNSTREAM": link.downstream,
        "LENGTH": link.length,
        "COSTS": link.costs,
        "LABEL": link.label,
    }


def dict_to_link(G: OrientedGraph, d: LinkDict) -> None:
    G.add_link(d["ID"],
               d["UPSTREAM"],
               d["DOWNSTREAM"],
               d["LENGTH"],
               d["COSTS"],
               d["LABEL"])


def graph_to_dict(G: OrientedGraph) -> GraphDict:
    nodes: list[NodeDict] = []
    links: list[LinkDict] = []

    for n in G.nodes.values():
        nodes.append(node_to_dict(n))

    for l in G.links.values():
        links.append(link_to_dict(l))

    return {"NODES": nodes, "LINKS": links}


def dict_to_graph(d: GraphDict) -> OrientedGraph:
    G = OrientedGraph()

    for n in d["NODES"]:
        dict_to_node(G, n)

    for l in d["LINKS"]:
        dict_to_link(G, l)

    return G
