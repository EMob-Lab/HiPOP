#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <hipop/shortest_path.h>

namespace py = pybind11;

namespace hipop_wrappers {

void shortest_path(py::module_ &m) {

    m.def(
        "dijkstra",
        &hipop::dijkstra,
        py::arg("graph"),
        py::arg("origin"),
        py::arg("destination"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("accessible_link_labels") = setstring()
    );

    m.def(
        "dijkstra_single_source",
        &hipop::dijkstraSingleSource,
        py::arg("graph"),
        py::arg("origin"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("accessible_link_labels") = setstring()
    );

    m.def(
        "floyd_warshall",
        &hipop::floydWarshall,
        py::arg("graph"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("accessible_link_labels") = setstring()
    );

    m.def(
        "parallel_dijkstra",
        &hipop::parallelDijkstra,
        py::arg("graph"),
        py::arg("origins"),
        py::arg("destinations"),
        py::arg("label_to_cost_family"),
        py::arg("cost_metric"),
        py::arg("thread_number"),
        py::arg("accessible_link_labels") = std::vector<setstring>()
    );

    m.def(
        "parallel_dijkstra_single_source",
        &hipop::parallelDijkstraSingleSource,
        py::arg("graph"),
        py::arg("origins"),
        py::arg("label_to_cost_family"),
        py::arg("cost_metric"),
        py::arg("thread_number"),
        py::arg("accessible_link_labels") = std::vector<setstring>()
    );

    m.def(
        "parallel_dijkstra_heterogeneous_costs",
        &hipop::parallelDijkstraHeterogeneousCosts,
        py::arg("graph"),
        py::arg("origins"),
        py::arg("destinations"),
        py::arg("label_to_cost_family"),
        py::arg("cost_metrics"),
        py::arg("thread_number"),
        py::arg("accessible_link_labels") = std::vector<setstring>()
    );

    m.def(
        "k_shortest_path",
        &hipop::KShortestPath,
        py::arg("graph"),
        py::arg("origin"),
        py::arg("destination"),
        py::arg("cost_metric"),
        py::arg("accessible_link_labels"),
        py::arg("label_to_cost_family"),
        py::arg("max_diff_cost"),
        py::arg("max_dist_in_common"),
        py::arg("cost_multiplier"),
        py::arg("max_retry"),
        py::arg("k_path"),
        py::arg("intermodal")
    );

    m.def(
        "parallel_k_shortest_path",
        &hipop::parallelKShortestPath,
        py::arg("graph"),
        py::arg("origins"),
        py::arg("destinations"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("accessible_link_labels"),
        py::arg("max_diff_cost"),
        py::arg("max_dist_in_common"),
        py::arg("cost_multiplier"),
        py::arg("max_retry"),
        py::arg("k_paths"),
        py::arg("thread_number")
    );

    m.def(
        "yen_k_shortest_path",
        &hipop::YenKShortestPath,
        py::arg("graph"),
        py::arg("origin"),
        py::arg("destination"),
        py::arg("cost_metric"),
        py::arg("accessible_link_labels"),
        py::arg("label_to_cost_family"),
        py::arg("k_path")
    );

    m.def(
        "astar_euclidian_dist",
        &hipop::aStarEuclidianDist,
        py::arg("graph"),
        py::arg("origin"),
        py::arg("destination"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("accessible_link_labels")
    );

    m.def(
        "compute_path_length",
        &hipop::computePathLength,
        py::arg("graph"),
        py::arg("path")
    );

    m.def(
        "compute_path_cost",
        &hipop::computePathCost,
        py::arg("graph"),
        py::arg("path"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family")
    );

    m.def(
        "compute_paths_costs",
        &hipop::computePathsCosts,
        py::arg("graph"),
        py::arg("paths"),
        py::arg("cost_metric"),
        py::arg("label_to_cost_family"),
        py::arg("thread_number")
    );

    m.def(
        "parallel_k_intermodal_shortest_path",
        &hipop::parallelKIntermodalShortestPath,
        py::arg("graph"),
        py::arg("origins"),
        py::arg("destinations"),
        py::arg("label_to_cost_family"),
        py::arg("cost_metric"),
        py::arg("thread_number"),
        py::arg("pair_mandatory_labels"),
        py::arg("max_diff_cost"),
        py::arg("max_dist_in_common"),
        py::arg("cost_multiplier"),
        py::arg("max_retry"),
        py::arg("k_paths"),
        py::arg("accessible_link_labels") = std::vector<setstring>()
    );
}

}
