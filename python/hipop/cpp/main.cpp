#include <nanobind/nanobind.h>

namespace nb = nanobind;

namespace hipop_wrappers {
    void graph(nb::module_ &);
    void shortest_path(nb::module_ &);
}

NB_MODULE(cpp, m) {

    nb::module_ graph = m.def_submodule("graph", "Graph module");
    hipop_wrappers::graph(graph);

    nb::module_ shortest_path = m.def_submodule("shortest_path", "Shortest path module");
    hipop_wrappers::shortest_path(shortest_path);

}
