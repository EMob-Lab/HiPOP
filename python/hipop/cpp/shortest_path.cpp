#include <hipop/graph.h>
#include <hipop/graph_path_cost.h>
#include <hipop/shortest_path.h>

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

void shortest_path(nb::module_ &m) {

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
        "Compute the shortest paths from the given origin to all the nodes reachable from it.\n"
        "\n"
        "WARNING: the Node and Link objects in the returned dict are non-owning references\n"
        "to data stored in the OrientedGraph. The graph must be kept alive for as long as they\n"
        "are used, and they must not be removed from it. Otherwise, accessing them results in\n"
        "undefined behavior (likely a crash).",
        nb::rv_policy::reference,
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
        "Compute, for each origin, the shortest paths to all the nodes reachable from it.\n"
        "\n"
        "WARNING: the Node and Link objects in the returned dicts are non-owning references\n"
        "to data stored in the OrientedGraph. The graph must be kept alive for as long as they\n"
        "are used, and they must not be removed from it. Otherwise, accessing them results in\n"
        "undefined behavior (likely a crash).",
        nb::rv_policy::reference,
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
