#include <hipop/graph.h>
#include <hipop/graph_path_cost.h>
#include <hipop/shortest_path.h>

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
using namespace nb::literals;

namespace hipop_wrappers {


/**
 * Adapter class, to make a SameOriginPathTree look like a set of destination `Node` objects.
 */
struct DestinationSet {

    static constexpr const char *doc =
        "Read-only, set-like view of the destination nodes reachable in a SameOriginPathTree\n"
        "(including the origin node itself).\n"
        "\n"
        "WARNING: the Node objects returned by the view are non-owning references to data stored\n"
        "in the OrientedGraph. The graph must be kept alive for as long as they are used, and they\n"
        "must not be removed from it. Otherwise, accessing them results in undefined behavior\n"
        "(likely a crash).";

    const hipop::SameOriginPathTree *tree; // never null
};


static void bind_same_origin_path_tree(nb::module_ &m) {

    nb::class_<hipop::SameOriginPathTree> treeClass(m, "SameOriginPathTree",
        "Set of paths sharing the same origin node, such as the shortest paths\n"
        "returned by dijkstra_single_source.\n"
        "\n"
        "WARNING: a tree holds references to the nodes and links of its graph.\n"
        "The graph must be kept alive as long as the tree is, and its nodes\n"
        "and links must not be removed from it. Otherwise, using the tree\n"
        "results in undefined behavior (likely a crash).");

    // Remark: the API exposed to the Python world for `DestinationSet` is set-like.
    // Still, it is not fully compliant with the `collections.abc.Set` contract
    // (no comparison operators, no set operations, equality is identity).
    nb::class_<DestinationSet>(treeClass, "DestinationSet", DestinationSet::doc)
        .def("__len__", [](const DestinationSet &d) { return d.tree->destinations().size(); })
        .def("__contains__", [](const DestinationSet &d, const hipop::Node *node) {
            return d.tree->hasPathTo(node);
        }, "node"_a)
        .def("__iter__", [](const DestinationSet &d) {
            auto range = d.tree->destinations();
            return nb::make_iterator<nb::rv_policy::reference>(nb::type<DestinationSet>(), "Iterator", range.begin(), range.end());
        }, nb::keep_alive<0, 1>());

    treeClass
        .def_prop_ro("origin", &hipop::SameOriginPathTree::origin, nb::rv_policy::reference)
        .def_prop_ro("destinations", [](const hipop::SameOriginPathTree &tree) {
            return DestinationSet{ &tree };
        }, nb::keep_alive<0, 1>())
        .def("has_path_to", &hipop::SameOriginPathTree::hasPathTo, "destination"_a)
        .def("path_to", &hipop::SameOriginPathTree::pathTo, "destination"_a)
        .def("find_path_to", &hipop::SameOriginPathTree::findPathTo, "destination"_a);
}


void shortest_path(nb::module_ &m) {

    bind_same_origin_path_tree(m);

    m.def(
        "dijkstra",
        &hipop::dijkstra,
        "graph"_a,
        "origin"_a,
        "destination"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "accessible_link_labels"_a = setstring()
    );

    m.def(
        "dijkstra_single_source",
        &hipop::dijkstraSingleSource,
        "graph"_a,
        "origin"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "accessible_link_labels"_a = setstring()
    );

    m.def(
        "floyd_warshall",
        &hipop::floydWarshall,
        "graph"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "accessible_link_labels"_a = setstring()
    );

    m.def(
        "parallel_dijkstra",
        &hipop::parallelDijkstra,
        "graph"_a,
        "origins"_a,
        "destinations"_a,
        "label_to_cost_family"_a,
        "cost_metric"_a,
        "thread_number"_a,
        "accessible_link_labels"_a = std::vector<setstring>()
    );

    m.def(
        "parallel_dijkstra_single_source",
        &hipop::parallelDijkstraSingleSource,
        "graph"_a,
        "origins"_a,
        "label_to_cost_family"_a,
        "cost_metric"_a,
        "thread_number"_a,
        "accessible_link_labels"_a = std::vector<setstring>()
    );

    m.def(
        "parallel_dijkstra_heterogeneous_costs",
        &hipop::parallelDijkstraHeterogeneousCosts,
        "graph"_a,
        "origins"_a,
        "destinations"_a,
        "label_to_cost_family"_a,
        "cost_metrics"_a,
        "thread_number"_a,
        "accessible_link_labels"_a = std::vector<setstring>()
    );

    m.def(
        "k_shortest_path",
        &hipop::KShortestPath,
        "graph"_a,
        "origin"_a,
        "destination"_a,
        "cost_metric"_a,
        "accessible_link_labels"_a,
        "label_to_cost_family"_a,
        "max_diff_cost"_a,
        "max_dist_in_common"_a,
        "cost_multiplier"_a,
        "max_retry"_a,
        "k_path"_a,
        "intermodal"_a
    );

    m.def(
        "parallel_k_shortest_path",
        &hipop::parallelKShortestPath,
        "graph"_a,
        "origins"_a,
        "destinations"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "accessible_link_labels"_a,
        "max_diff_cost"_a,
        "max_dist_in_common"_a,
        "cost_multiplier"_a,
        "max_retry"_a,
        "k_paths"_a,
        "thread_number"_a
    );

    m.def(
        "yen_k_shortest_path",
        &hipop::YenKShortestPath,
        "graph"_a,
        "origin"_a,
        "destination"_a,
        "cost_metric"_a,
        "accessible_link_labels"_a,
        "label_to_cost_family"_a,
        "k_path"_a
    );

    m.def(
        "astar_euclidian_dist",
        &hipop::aStarEuclidianDist,
        "graph"_a,
        "origin"_a,
        "destination"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "accessible_link_labels"_a
    );

    m.def(
        "compute_path_length",
        &hipop::computePathLength,
        "path"_a
    );

    m.def(
        "compute_path_cost",
        &hipop::computePathCost,
        "path"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a
    );

    m.def(
        "compute_paths_costs",
        &hipop::computePathsCosts,
        "paths"_a,
        "cost_metric"_a,
        "label_to_cost_family"_a,
        "thread_number"_a
    );

    m.def(
        "parallel_k_intermodal_shortest_path",
        &hipop::parallelKIntermodalShortestPath,
        "graph"_a,
        "origins"_a,
        "destinations"_a,
        "label_to_cost_family"_a,
        "cost_metric"_a,
        "thread_number"_a,
        "pair_mandatory_labels"_a,
        "max_diff_cost"_a,
        "max_dist_in_common"_a,
        "cost_multiplier"_a,
        "max_retry"_a,
        "k_paths"_a,
        "accessible_link_labels"_a = std::vector<setstring>()
    );
}

}
