#include <hipop/create.h>
#include <hipop/graph.h>
#include <hipop/graph_path.h>
#include <hipop/iterator_util.h>

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <optional>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

namespace nb = nanobind;
using namespace nb::literals;

namespace hipop_wrappers {


/**
 * Adapter class, to make an OrientedGraph class look like a container of `Node` objects.
 */
struct NodeMapping {

    using Element = hipop::Node;
    using Range = hipop::OrientedGraph::NodeRange;

    static constexpr const char *doc =
        "Read-only, dictionary-like view of the nodes of an OrientedGraph, indexed by node ID.\n"
        "\n"
        "WARNINGS:\n"
        "- The view reflects the current state of the graph. Adding or removing nodes\n"
        "  while iterating over the view (or over its keys/values/items) results in undefined\n"
        "  behavior (likely a crash).\n"
        "- The Node objects returned by the view are non-owning references to data stored\n"
        "  in the OrientedGraph. The graph must be kept alive for as long as they are used, and they\n"
        "  must not be removed from it. Otherwise, accessing them results in undefined behavior\n"
        "  (likely a crash).";

    struct KeyView { hipop::OrientedGraph *graph; };
    struct ValueView { hipop::OrientedGraph *graph; };
    struct ItemView { hipop::OrientedGraph *graph; };

    hipop::OrientedGraph *graph; // never null

    [[nodiscard]] static std::size_t count(const hipop::OrientedGraph &graph) { return graph.nodeCount(); }
    [[nodiscard]] static hipop::Node *find(hipop::OrientedGraph &graph, const std::string &id) { return graph.findNode(id); }
    [[nodiscard]] static hipop::OrientedGraph::NodeRange range(hipop::OrientedGraph &graph) { return graph.nodes(); }
};


/**
 * Adapter class, to make an OrientedGraph class look like a container of `Link` objects.
 */
struct LinkMapping {

    using Element = hipop::Link;
    using Range = hipop::OrientedGraph::LinkRange;

    static constexpr const char *doc =
        "Read-only, dictionary-like view of the links of an OrientedGraph, indexed by link ID.\n"
        "\n"
        "WARNINGS:\n"
        "- The view reflects the current state of the graph. Adding or removing links\n"
        "  (or nodes) while iterating over the view (or over its keys/values/items) results in\n"
        "  undefined behavior (likely a crash).\n"
        "- The Link objects returned by the view are non-owning references to data stored\n"
        "  in the OrientedGraph. The graph must be kept alive for as long as they are used, and they\n"
        "  must not be removed from it. Otherwise, accessing them results in undefined behavior\n"
        "  (likely a crash).";

    struct KeyView { hipop::OrientedGraph *graph; };
    struct ValueView { hipop::OrientedGraph *graph; };
    struct ItemView { hipop::OrientedGraph *graph; };

    hipop::OrientedGraph *graph; // never null

    [[nodiscard]] static std::size_t count(const hipop::OrientedGraph &graph) { return graph.linkCount(); }
    [[nodiscard]] static hipop::Link *find(hipop::OrientedGraph &graph, const std::string &id) { return graph.findLink(id); }
    [[nodiscard]] static hipop::OrientedGraph::LinkRange range(hipop::OrientedGraph &graph) { return graph.links(); }
};


/**
 * Register the Python class `name` corresponding to `Mapping` (`NodeMapping` or `LinkMapping`).
 *
 * Remark: the API exposed to the Python world for the `Mapping` class is dictionary-like.
 * Still, it is not fully compliant with the `collections.abc.Mapping` contract:
 *  - `get()` does not accept a default value,
 *  - `__eq__` is not implemented (equality is identity).
 */
template<typename Mapping>
static void bind_graph_element_mapping(nb::module_ &m, const char *name) {

    using Element = typename Mapping::Element;
    using KeyView = typename Mapping::KeyView;
    using ValueView = typename Mapping::ValueView;
    using ItemView = typename Mapping::ItemView;

    // Auxiliary classes for the .keys() / .items() iterators
    // Remark: no projection is needed for the .values() iterator, as its value-type corresponds
    // to the value-type of the underlying C++ range/iterator objects.
    struct KeyProjection {
        const std::string &operator()(Element *element) const {
            return element->id();
        }
    };
    struct ItemProjection {
        std::pair<const std::string &, Element *> operator()(Element *element) const {
            return { element->id(), element };
        }
    };
    using KeyIterator = hipop::ProjectingIterator<typename Mapping::Range::iterator, KeyProjection>;
    using ItemIterator = hipop::ProjectingIterator<typename Mapping::Range::iterator, ItemProjection>;

    nb::class_<Mapping> mappingClass(m, name, Mapping::doc);

    nb::class_<KeyView>(mappingClass, "KeyView")
        .def("__len__", [](const KeyView &v) { return Mapping::count(*v.graph); })
        .def("__contains__", [](const KeyView &v, const std::string &id) {
            return Mapping::find(*v.graph, id) != nullptr;
        })
        .def("__iter__", [](const KeyView &v) {
            auto range = Mapping::range(*v.graph);
            return nb::make_iterator(nb::type<Mapping>(), "KeyIterator",
                KeyIterator(range.begin()), KeyIterator(range.end()));
        }, nb::keep_alive<0, 1>());

    nb::class_<ValueView>(mappingClass, "ValueView")
        .def("__len__", [](const ValueView &v) { return Mapping::count(*v.graph); })
        .def("__iter__", [](const ValueView &v) {
            auto range = Mapping::range(*v.graph);
            return nb::make_iterator<nb::rv_policy::reference>(nb::type<Mapping>(), "ValueIterator",
                range.begin(), range.end());
        }, nb::keep_alive<0, 1>());

    nb::class_<ItemView>(mappingClass, "ItemView")
        .def("__len__", [](const ItemView &v) { return Mapping::count(*v.graph); })
        .def("__iter__", [](const ItemView &v) {
            auto range = Mapping::range(*v.graph);
            return nb::make_iterator<nb::rv_policy::reference>(nb::type<Mapping>(), "ItemIterator",
                ItemIterator(range.begin()), ItemIterator(range.end()));
        }, nb::keep_alive<0, 1>());

    mappingClass
        .def("__len__", [](const Mapping &v) { return Mapping::count(*v.graph); })
        .def("__contains__", [](const Mapping &v, const std::string &id) {
            return Mapping::find(*v.graph, id) != nullptr;
        })
        .def("__getitem__", [](const Mapping &v, const std::string &id) {
            Element *element = Mapping::find(*v.graph, id);
            if (element == nullptr) {
                throw nb::key_error(id.c_str());
            }
            return element;
        }, "id"_a, nb::rv_policy::reference)
        .def("get", [](const Mapping &v, const std::string &id) {
            Element *element = Mapping::find(*v.graph, id);
            return element == nullptr ? std::nullopt : std::make_optional(element);
        }, "id"_a, nb::rv_policy::reference)
        .def("__iter__", [](const Mapping &v) {
            auto range = Mapping::range(*v.graph);
            return nb::make_iterator(nb::type<Mapping>(), "KeyIterator",
                KeyIterator(range.begin()), KeyIterator(range.end()));
        }, nb::keep_alive<0, 1>())
        .def("keys", [](const Mapping &v) { return KeyView{ v.graph }; }, nb::keep_alive<0, 1>())
        .def("values", [](const Mapping &v) { return ValueView{ v.graph }; }, nb::keep_alive<0, 1>())
        .def("items", [](const Mapping &v) { return ItemView{ v.graph }; }, nb::keep_alive<0, 1>());
}


/**
 * Adapter class, to make an OrientedGraphPath class look like a sequence of `Node` objects.
 */
struct PathNodeSequence {

    static constexpr const char *doc =
        "Read-only, list-like view of the nodes visited by an OrientedGraphPath, from origin to destination.\n"
        "Its length is the number of links in the path + 1 (in particular, it is never empty).\n"
        "\n"
        "WARNINGS:\n"
        "- The view reflects the current state of the path. Modifying the path while iterating\n"
        "  over the view results in undefined behavior (likely a crash).\n"
        "- The Node objects returned by the view are non-owning references to data stored\n"
        "  in the OrientedGraph. The graph must be kept alive for as long as they are used, and they\n"
        "  must not be removed from it. Otherwise, accessing them results in undefined behavior\n"
        "  (likely a crash).";

    const hipop::OrientedGraphPath *path; // never null

    [[nodiscard]] static std::size_t count(const hipop::OrientedGraphPath &path) { return path.size() + 1; }
    [[nodiscard]] static const hipop::Node *at(const hipop::OrientedGraphPath &path, std::size_t index) { return path.node(index); }
    [[nodiscard]] static hipop::OrientedGraphPath::NodeRange range(const hipop::OrientedGraphPath &path) { return path.nodes(); }
    [[nodiscard]] static std::vector<std::string> ids(const hipop::OrientedGraphPath &path) { return path.nodeIds(); }
};


/**
 * Adapter class, to make an OrientedGraphPath class look like a sequence of `Link` objects.
 */
struct PathLinkSequence {

    static constexpr const char *doc =
        "Read-only, list-like view of the links visited by an OrientedGraphPath, from origin to destination.\n"
        "\n"
        "WARNINGS:\n"
        "- The view reflects the current state of the path. Modifying the path while iterating\n"
        "  over the view results in undefined behavior (likely a crash).\n"
        "- The Link objects returned by the view are non-owning references to data stored\n"
        "  in the OrientedGraph. The graph must be kept alive for as long as they are used, and they\n"
        "  must not be removed from it. Otherwise, accessing them results in undefined behavior\n"
        "  (likely a crash).";

    const hipop::OrientedGraphPath *path; // never null

    [[nodiscard]] static std::size_t count(const hipop::OrientedGraphPath &path) { return path.size(); }
    [[nodiscard]] static const hipop::Link *at(const hipop::OrientedGraphPath &path, std::size_t index) { return path.link(index); }
    [[nodiscard]] static const std::vector<const hipop::Link *> &range(const hipop::OrientedGraphPath &path) { return path.links(); }
    [[nodiscard]] static std::vector<std::string> ids(const hipop::OrientedGraphPath &path) { return path.linkIds(); }
};


/**
 * Register the Python class `name` corresponding to `Sequence` (`PathNodeSequence` or `PathLinkSequence`).
 *
 * Remark: the API exposed to the Python world for the `Sequence` class is list-like.
 * Still, it is not fully compliant with the `collections.abc.Sequence` contract:
 *  - slicing is not supported,
 *  - `index()` and `count()` are not implemented,
 *  - `__contains__` is not implemented (the `in` operator falls back to iterating over the sequence),
 *  - `__eq__` is not implemented (equality is identity).
 */
template<typename Sequence>
static void bind_path_element_sequence(nb::module_ &m, const char *name) {

    nb::class_<Sequence>(m, name, Sequence::doc)
        .def("__len__", [](const Sequence &s) { return Sequence::count(*s.path); })
        .def("__getitem__", [](const Sequence &s, std::ptrdiff_t index) {
            const auto count = static_cast<std::ptrdiff_t>(Sequence::count(*s.path));
            if (index < 0) {
                index += count; // Python-style negative indexing.
            }
            if (index < 0 || index >= count) {
                throw nb::index_error();
            }
            return Sequence::at(*s.path, static_cast<std::size_t>(index));
        }, "index"_a, nb::rv_policy::reference)
        .def("__iter__", [](const Sequence &s) {
            decltype(auto) range = Sequence::range(*s.path);
            return nb::make_iterator<nb::rv_policy::reference>(nb::type<Sequence>(), "Iterator", range.begin(), range.end());
        }, nb::keep_alive<0, 1>())
        .def("__reversed__", [](const Sequence &s) {
            decltype(auto) range = Sequence::range(*s.path);
            return nb::make_iterator<nb::rv_policy::reference>(nb::type<Sequence>(), "ReverseIterator", range.rbegin(), range.rend());
        }, nb::keep_alive<0, 1>())
        .def("ids", [](const Sequence &s) { return Sequence::ids(*s.path); }); // Return a plain Python list.
}


void graph(nb::module_ &m) {

    nb::class_<hipop::Link>(m, "Link",
        "Oriented link between two nodes of an OrientedGraph.\n"
        "\n"
        "WARNING: a Link object is a non-owning reference to data stored\n"
        "in the OrientedGraph it belongs to. The graph must be kept alive\n"
        "for as long as the link is used, and the link must not be removed\n"
        "from it. Otherwise, accessing the link results in undefined behavior\n"
        "(likely a crash).")
        .def_prop_ro("id", &hipop::Link::id)
        .def_prop_ro("upstream", [](const hipop::Link &link) { return link.up()->id(); })
        .def_prop_ro("downstream", [](const hipop::Link &link) { return link.down()->id(); })
        .def_prop_ro("length", &hipop::Link::length)
        .def_prop_ro("costs", &hipop::Link::costs)
        .def_prop_ro("label", &hipop::Link::label)
        .def("update_costs", &hipop::Link::setCosts, "costs"_a);

    nb::class_<hipop::Node>(m, "Node",
        "Node of an OrientedGraph.\n"
        "\n"
        "WARNING: a Node object is a non-owning reference to data stored\n"
        "in the OrientedGraph it belongs to. The graph must be kept alive\n"
        "for as long as the node is used, and the node must not be removed\n"
        "from it. Otherwise, accessing the node results in undefined behavior\n"
        "(likely a crash).")
        .def_prop_ro("id", &hipop::Node::id)
        .def_prop_ro("position", &hipop::Node::position)
        .def_prop_ro("adj", &hipop::Node::adj)
        .def_prop_ro("radj", &hipop::Node::radj)
        .def_prop_ro("label", &hipop::Node::label)
        .def_prop_ro("exclude_movements", &hipop::Node::excludeMovements)
        .def("get_exits", &hipop::Node::getExits, "predecessor"_a, nb::rv_policy::reference);

    bind_graph_element_mapping<NodeMapping>(m, "NodeMapping");
    bind_graph_element_mapping<LinkMapping>(m, "LinkMapping");

    nb::class_<hipop::OrientedGraph>(m, "OrientedGraph")
        .def(nb::init<>())

        // Node/link accessors
        // Remark: on the Python side, nodes and links are accessed only through the read-only mapping views
        // returned by `nodes` and `links`, the other C++ accessors (`node()`, `findNode()`, etc.) are not exposed.
        .def_prop_ro("nodes", [](hipop::OrientedGraph &graph) {
            return NodeMapping{ &graph };
        }, nb::keep_alive<0, 1>())
        .def_prop_ro("links", [](hipop::OrientedGraph &graph) {
            return LinkMapping{ &graph };
        }, nb::keep_alive<0, 1>())

        // Copy
        .def("copy", [](const hipop::OrientedGraph &graph) {
            return hipop::OrientedGraph(graph);
        })
        .def("__copy__", [](const hipop::OrientedGraph &graph) {
            return hipop::OrientedGraph(graph);
        })
        .def("__deepcopy__", [](const hipop::OrientedGraph &graph, const nb::typed<nb::dict, int, nb::any> &) {
            // The graph owns its nodes and links (it is not a container of shared references to them),
            // so copying the graph always copies them: thus, __copy__ and __deepcopy__ behave identically.
            return hipop::OrientedGraph(graph);
        }, "memo"_a)

        // Other operations
        .def("add_all_nodes_and_links", &hipop::OrientedGraph::AddAllNodesAndLinks, "graph"_a)
        .def("add_node", &hipop::OrientedGraph::AddNode,
            "node_id"_a, "x"_a, "y"_a, "label"_a, "exclude_movements"_a = mapsets())
        .def("add_link", &hipop::OrientedGraph::AddLink,
            "link_id"_a, "up"_a, "down"_a, "length"_a, "costs"_a, "label"_a = "_def")
        .def("delete_node", &hipop::OrientedGraph::DeleteNode, "node_id"_a)
        .def("delete_link", &hipop::OrientedGraph::DeleteLink, "link_id"_a)
        .def("update_costs", &hipop::OrientedGraph::UpdateCosts, "link_id_to_costs"_a)
        .def("get_links_without_cost", &hipop::OrientedGraph::GetLinksWithoutCost,
            "cost_metric"_a, "label_to_cost_family"_a);

    bind_path_element_sequence<PathNodeSequence>(m, "PathNodeSequence");
    bind_path_element_sequence<PathLinkSequence>(m, "PathLinkSequence");

    nb::class_<hipop::OrientedGraphPath>(m, "OrientedGraphPath",
        "A path within an OrientedGraph, i.e. a sequence of consecutive links\n"
        "from an origin node to a destination node.\n"
        "\n"
        "WARNING: a path holds references to the nodes and links of its graph.\n"
        "The graph must be kept alive as long as the path is, and the nodes\n"
        "and links visited by the path must not be removed from it. Otherwise,\n"
        "using the path results in undefined behavior (likely a crash).")
        .def(nb::init<const hipop::Node *>(), "origin"_a)
        .def(nb::init<const hipop::Node *, std::vector<const hipop::Link *>>(), "origin"_a, "links"_a)

        // Node/link accessors
        // Remark: on the Python side, nodes and links are accessed only through the read-only sequence views
        // returned by `nodes` and `links`, the other C++ accessors (`node()`, `link()`, etc.) are not exposed.
        .def_prop_ro("origin", &hipop::OrientedGraphPath::origin, nb::rv_policy::reference)
        .def_prop_ro("destination", &hipop::OrientedGraphPath::destination, nb::rv_policy::reference)
        .def_prop_ro("nodes", [](const hipop::OrientedGraphPath &path) {
            return PathNodeSequence{ &path };
        }, nb::keep_alive<0, 1>())
        .def_prop_ro("links", [](const hipop::OrientedGraphPath &path) {
            return PathLinkSequence{ &path };
        }, nb::keep_alive<0, 1>())

        // Copy
        .def("copy", [](const hipop::OrientedGraphPath &path) {
            return hipop::OrientedGraphPath(path);
        })
        .def("__copy__", [](const hipop::OrientedGraphPath &path) {
            return hipop::OrientedGraphPath(path);
        })
        .def("__deepcopy__", [](const hipop::OrientedGraphPath &path, const nb::typed<nb::dict, int, nb::any> &) {
            // Nodes and links belong to the graph, not to the path: they must not be copied, even for a deep copy.
            // Thus, same behavior for __copy__ and __deepcopy__ in case of OrientedGraphPath.
            return hipop::OrientedGraphPath(path);
        }, "memo"_a)

        // Other operations
        .def("has_loop", &hipop::OrientedGraphPath::hasLoop)
        .def("add", nb::overload_cast<const hipop::Link *>(&hipop::OrientedGraphPath::add), "link"_a)
        .def("add", nb::overload_cast<const hipop::OrientedGraphPath &>(&hipop::OrientedGraphPath::add), "path"_a)
        .def("sub_path", [](const hipop::OrientedGraphPath &path, std::size_t pos, std::optional<std::size_t> linkCount) {
            return linkCount.has_value() ? path.subPath(pos, *linkCount) : path.subPath(pos);
        }, "pos"_a, "link_count"_a = nb::none())
        .def("starts_with", &hipop::OrientedGraphPath::startsWith, "prefix"_a)
        .def("ends_with", &hipop::OrientedGraphPath::endsWith, "suffix"_a)

        // Operators and special methods.
        .def("__repr__", [](const hipop::OrientedGraphPath &path) {
            std::ostringstream stream;
            stream << "OrientedGraphPath(" << path << ")";
            return stream.str();
        })
        .def("__eq__", [](const hipop::OrientedGraphPath &path, nb::handle other) {
            return nb::isinstance<hipop::OrientedGraphPath>(other) && path == nb::cast<const hipop::OrientedGraphPath &>(other);
        }, "other"_a.none())
        // Remark: operator != is not bound, as Python deduces it from operator ==.
        .attr("__hash__") = nb::none(); // Make obj unhashable, as it is mutable.

    m.def("generate_manhattan", &hipop::makeManhattan, "n"_a, "link_length"_a);

    m.def("make_empty_path", &hipop::makeEmptyPath, "graph"_a, "origin_node_id"_a);

    m.def("make_path", &hipop::makePath, "graph"_a, "link_ids"_a);

    m.def("merge_oriented_graph", &hipop::mergeOrientedGraph, "graphs"_a);
}

}
