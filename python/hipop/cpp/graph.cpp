#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>
#include <nanobind/operators.h>

#include <hipop/create.h>
#include <hipop/graph.h>
#include <hipop/graph_path.h>

#include <limits>
#include <sstream>

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

    nb::class_<hipop::OrientedGraphPath>(m, "OrientedGraphPath",
        "A path within an OrientedGraph, i.e. a sequence of consecutive links from an origin node "
        "to a destination node.\n\n"
        "WARNING: a path holds references to the nodes and links of its graph. The graph must be kept alive "
        "as long as the path is, and the nodes and links visited by the path must not be removed from it. "
        "Otherwise, using the path results in undefined behavior (likely a crash).")
        .def(nb::init<const hipop::Node *>(), "origin"_a)
        .def(nb::init<const hipop::Node *, std::vector<const hipop::Link *>>(), "origin"_a, "links"_a)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__len__", &hipop::OrientedGraphPath::size)
        .def("__bool__", [](const hipop::OrientedGraphPath &) {
            // An empty path must be not treated as an empty container, even if `__len__()` returns 0.
            return true;
        })
        .def("__copy__", [](const hipop::OrientedGraphPath &path) {
            return hipop::OrientedGraphPath(path);
        })
        .def("__deepcopy__", [](const hipop::OrientedGraphPath &path, const nb::dict &) {
            // Nodes and links belong to the graph, not to the path: they must not be copied, even for a deep copy.
            // Thus, same behavior for __copy__ and __deepcopy__ in case of OrientedGraphPath.
            return hipop::OrientedGraphPath(path);
        }, "memo"_a)
        .def("__repr__", [](const hipop::OrientedGraphPath &path) {
            std::ostringstream stream;
            stream << "OrientedGraphPath(" << path << ")";
            return stream.str();
        })
        .def_prop_ro("origin", &hipop::OrientedGraphPath::origin, nb::rv_policy::reference)
        .def_prop_ro("origin_id", &hipop::OrientedGraphPath::originId)
        .def_prop_ro("destination", &hipop::OrientedGraphPath::destination, nb::rv_policy::reference)
        .def_prop_ro("destination_id", &hipop::OrientedGraphPath::destinationId)
        .def("node", &hipop::OrientedGraphPath::node, "node_index"_a, nb::rv_policy::reference)
        .def("node_id", &hipop::OrientedGraphPath::nodeId, "node_index"_a)
        .def("link", &hipop::OrientedGraphPath::link, "link_index"_a, nb::rv_policy::reference)
        .def("link_id", &hipop::OrientedGraphPath::linkId, "link_index"_a)
        .def_prop_ro("nodes", [](const hipop::OrientedGraphPath &path) {
            auto range = path.nodes();
            return std::vector<const hipop::Node *>(range.begin(), range.end());
        }, nb::rv_policy::reference)
        .def_prop_ro("node_ids", &hipop::OrientedGraphPath::nodeIds)
        .def_prop_ro("links", &hipop::OrientedGraphPath::links, nb::rv_policy::reference)
        .def_prop_ro("link_ids", &hipop::OrientedGraphPath::linkIds)
        .def("add", nb::overload_cast<const hipop::Link *>(&hipop::OrientedGraphPath::add), "link"_a)
        .def("add", nb::overload_cast<const hipop::OrientedGraphPath &>(&hipop::OrientedGraphPath::add), "other"_a)
        .def("sub_path", &hipop::OrientedGraphPath::subPath,
            "pos"_a, "link_count"_a = std::numeric_limits<std::size_t>::max())
        .def("starts_with", &hipop::OrientedGraphPath::startsWith, "p"_a)
        .def("ends_with", &hipop::OrientedGraphPath::endsWith, "p"_a);

    m.def("generate_manhattan", &hipop::makeManhattan, "n"_a, "link_length"_a);

    m.def("make_empty_path", &hipop::makeEmptyPath, "graph"_a, "origin_node_id"_a);

    m.def("make_path", &hipop::makePath, "graph"_a, "link_ids"_a);

    m.def("merge_oriented_graph", &hipop::mergeOrientedGraph, "graphs"_a);

    m.def("copy_graph", [](const hipop::OrientedGraph &graph) {
        return new hipop::OrientedGraph(graph);
    }, "graph"_a);
}

}
