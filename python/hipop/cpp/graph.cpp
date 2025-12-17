#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <hipop/graph.h>
#include <hipop/create.h>

namespace py = pybind11;

namespace hipop_wrappers {

void graph(py::module_ &m) {
    py::class_<hipop::Link>(m, "Link")
        .def(py::init<std::string, std::string, std::string, double, mapcosts,  std::string>(),
             py::arg("id"), py::arg("up"), py::arg("down"), py::arg("length"), py::arg("cost"), py::arg("label") = "")
        .def_readonly("id", &hipop::Link::mid)
        .def_readonly("upstream", &hipop::Link::mupstream)
        .def_readonly("downstream", &hipop::Link::mdownstream)
        .def_readonly("costs", &hipop::Link::mcosts)
        .def_readonly("label", &hipop::Link::mlabel)
        .def_readonly("length", &hipop::Link::mlength)
        .def("update_costs", &hipop::Link::updateCosts);

    py::class_<hipop::Node>(m, "Node")
          .def(py::init<std::string, double, double, std::string, std::unordered_map<std::string, std::set<std::string> > >(),
               py::arg("id"), py::arg("x"), py::arg("y"), py::arg("label") = "",py::arg("exclude_movements") = mapsets())
          .def_readonly("id", &hipop::Node::mid)
          .def_readonly("position", &hipop::Node::mposition)
          .def_readonly("adj", &hipop::Node::madj)
          .def_readonly("radj", &hipop::Node::mradj)
          .def_readonly("label", &hipop::Node::mlabel)
          .def_readonly("exclude_movements", &hipop::Node::mexclude_movements)
          .def("get_exits", &hipop::Node::getExits, py::arg("predecessor")="_default", py::return_value_policy::reference)
          .def("get_entrances", &hipop::Node::getEntrances, py::arg("predecessor")="_default", py::return_value_policy::reference);

    py::class_<hipop::OrientedGraph>(m, "OrientedGraph")
          .def(py::init<>())
          .def_readwrite("nodes", &hipop::OrientedGraph::mnodes)
          .def_readwrite("links", &hipop::OrientedGraph::mlinks)
          .def("add_node", py::overload_cast<std::string, double, double, std::string, mapsets>(&hipop::OrientedGraph::AddNode), py::arg("id"), py::arg("x"), py::arg("y"), py::arg("label"), py::arg("exclude_movements") = mapsets())
          .def("add_node", py::overload_cast<hipop::Node*>(&hipop::OrientedGraph::AddNode), py::arg("node"))
          .def("add_link", py::overload_cast<std::string, std::string, std::string, double, mapcosts, std::string>(&hipop::OrientedGraph::AddLink),
               py::arg("id"), py::arg("up"), py::arg("down"), py::arg("length"), py::arg("costs"), py::arg("label") = "_def")
          .def("delete_link",&hipop::OrientedGraph::DeleteLink)
          .def("delete_all_links_to_node",&hipop::OrientedGraph::DeleteAllLinksToNode)
          .def("get_link", &hipop::OrientedGraph::getLink)
          .def("update_link_costs", &hipop::OrientedGraph::UpdateLinkCosts)
          .def("update_costs", &hipop::OrientedGraph::UpdateCosts)
          .def("get_length", &hipop::OrientedGraph::getLength)
          .def("get_links_without_cost", &hipop::OrientedGraph::GetLinksWithoutCost);

    m.def("generate_manhattan", &hipop::makeManhattan);

    m.def("merge_oriented_graph", &hipop::mergeOrientedGraph);

    m.def("copy_graph", &hipop::copyGraph);
}

}
