#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include <hipop/graph.h>
#include <hipop/create.h>

namespace nb = nanobind;
using namespace nb::literals;

namespace hipop_wrappers {

void graph(nb::module_ &m) {

    nb::class_<hipop::Link>(m, "Link")
        .def_ro("id", &hipop::Link::mid)
        .def_prop_ro("upstream", [](const hipop::Link &link) { return link.mup->mid; })
        .def_prop_ro("downstream", [](const hipop::Link &link) { return link.mdown->mid; })
        .def_ro("costs", &hipop::Link::mcosts)
        .def_ro("label", &hipop::Link::mlabel)
        .def_ro("length", &hipop::Link::mlength)
        .def("update_costs", &hipop::Link::updateCosts, "costs"_a);

    nb::class_<hipop::Node>(m, "Node")
        .def_ro("id", &hipop::Node::mid)
        .def_ro("position", &hipop::Node::mposition)
        .def_ro("adj", &hipop::Node::madj)
        .def_ro("radj", &hipop::Node::mradj)
        .def_ro("label", &hipop::Node::mlabel)
        .def_ro("exclude_movements", &hipop::Node::mexclude_movements)
        .def("get_exits", &hipop::Node::getExits, "predecessor"_a, nb::rv_policy::reference);

    nb::class_<hipop::OrientedGraph>(m, "OrientedGraph")
        .def(nb::init<>())
        .def_rw("nodes", &hipop::OrientedGraph::mnodes)
        .def_rw("links", &hipop::OrientedGraph::mlinks)
        .def("add_all_nodes_and_links", &hipop::OrientedGraph::AddAllNodesAndLinks, "graph"_a)
        .def("add_node", &hipop::OrientedGraph::AddNode,
            "id"_a, "x"_a, "y"_a, "label"_a, "exclude_movements"_a = mapsets())
        .def("add_link", &hipop::OrientedGraph::AddLink,
            "id"_a, "up"_a, "down"_a, "length"_a, "costs"_a, "label"_a = "_def")
        .def("delete_link", &hipop::OrientedGraph::DeleteLink, "link_id"_a)
        .def("delete_all_links_to_node", &hipop::OrientedGraph::DeleteAllLinksToNode, "node_id"_a)
        .def("get_link", &hipop::OrientedGraph::getLink, "link_id"_a, nb::rv_policy::reference)
        .def("update_link_costs", &hipop::OrientedGraph::UpdateLinkCosts, "link_id"_a, "costs"_a)
        .def("update_costs", &hipop::OrientedGraph::UpdateCosts, "link_id_to_costs"_a)
        .def("get_length", &hipop::OrientedGraph::getLength, "up"_a, "down"_a)
        .def("get_links_without_cost", &hipop::OrientedGraph::GetLinksWithoutCost,
            "cost_metric"_a, "label_to_cost_family"_a);

    m.def("generate_manhattan", &hipop::makeManhattan, "n"_a, "link_length"_a);

    m.def("merge_oriented_graph", &hipop::mergeOrientedGraph, "graphs"_a);

    m.def("copy_graph", [](const hipop::OrientedGraph &graph) {
        return new hipop::OrientedGraph(graph);
    }, "graph"_a);
}

}
