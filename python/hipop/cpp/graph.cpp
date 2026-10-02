#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <hipop/graph.h>
#include <hipop/create.h>

namespace py = pybind11;

namespace hipop_wrappers {

void graph(py::module_ &m) {

    py::class_<hipop::Link>(m, "Link")
        .def_readonly("id", &hipop::Link::mid)
        .def_property_readonly("upstream", [](const hipop::Link &link) { return link.mup->mid; })
        .def_property_readonly("downstream", [](const hipop::Link &link) { return link.mdown->mid; })
        .def_readonly("costs", &hipop::Link::mcosts)
        .def_readonly("label", &hipop::Link::mlabel)
        .def_readonly("length", &hipop::Link::mlength)
        .def("update_costs", &hipop::Link::updateCosts, py::arg("costs"));

    py::class_<hipop::Node>(m, "Node")
        .def_readonly("id", &hipop::Node::mid)
        .def_readonly("position", &hipop::Node::mposition)
        .def_readonly("adj", &hipop::Node::madj)
        .def_readonly("radj", &hipop::Node::mradj)
        .def_readonly("label", &hipop::Node::mlabel)
        .def_readonly("exclude_movements", &hipop::Node::mexclude_movements)
        .def("get_exits", &hipop::Node::getExits, py::arg("predecessor"), py::return_value_policy::reference);

    py::class_<hipop::OrientedGraph>(m, "OrientedGraph")
        .def(py::init<>())
        .def_readwrite("nodes", &hipop::OrientedGraph::mnodes)
        .def_readwrite("links", &hipop::OrientedGraph::mlinks)
        .def("add_all_nodes_and_links", &hipop::OrientedGraph::AddAllNodesAndLinks, py::arg("graph"))
        .def("add_node", &hipop::OrientedGraph::AddNode,
            py::arg("id"), py::arg("x"), py::arg("y"), py::arg("label"), py::arg("exclude_movements") = mapsets())
        .def("add_link", &hipop::OrientedGraph::AddLink,
            py::arg("id"), py::arg("up"), py::arg("down"), py::arg("length"), py::arg("costs"), py::arg("label") = "_def")
        .def("delete_link", &hipop::OrientedGraph::DeleteLink, py::arg("link_id"))
        .def("delete_all_links_to_node", &hipop::OrientedGraph::DeleteAllLinksToNode, py::arg("node_id"))
        .def("get_link", &hipop::OrientedGraph::getLink, py::arg("link_id"), py::return_value_policy::reference)
        .def("update_link_costs", &hipop::OrientedGraph::UpdateLinkCosts, py::arg("link_id"), py::arg("costs"))
        .def("update_costs", &hipop::OrientedGraph::UpdateCosts, py::arg("link_id_to_costs"))
        .def("get_length", &hipop::OrientedGraph::getLength, py::arg("up"), py::arg("down"))
        .def("get_links_without_cost", &hipop::OrientedGraph::GetLinksWithoutCost,
            py::arg("cost_metric"), py::arg("label_to_cost_family"));

    m.def("generate_manhattan", &hipop::makeManhattan, py::arg("n"), py::arg("link_length"));

    m.def("merge_oriented_graph", &hipop::mergeOrientedGraph, py::arg("graphs"));

    m.def("copy_graph", [](const hipop::OrientedGraph &graph) {
        return new hipop::OrientedGraph(graph);
    }, py::arg("graph"));
}

}
