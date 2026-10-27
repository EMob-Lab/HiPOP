#pragma once

#include <hipop/string_util.h>

#include <array>
#include <cstddef>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

using setstring = std::set<std::string>;
using vecstring = std::vector<std::string>;
using mapsets = std::unordered_map<std::string, std::set<std::string>>;
using mapcosts = std::unordered_map<std::string, std::unordered_map<std::string, double>>;


namespace hipop
{
    class Node;


    class Link {
    public:
        std::string mid;
        const Node *mup;
        const Node *mdown;
        mapcosts mcosts; // mcosts[cost-family][cost-metric] -> cost-value
        std::string mlabel;
        double mlength;

    private:

        Link(std::string id, const Node *up, const Node *down, double length, mapcosts costs, std::string label = "") :
            mid(std::move(id)),
            mup(up),
            mdown(down),
            mcosts(std::move(costs)),
            mlabel(std::move(label)),
            mlength(length)
        {}

        friend class OrientedGraph;

    public:

        // Relationships between Node and Link instances are materialized through raw pointers.
        // Copying and/or moving these instances would break these relationships.
        Link(const Link &other) = delete;
        Link(Link &&other) = delete;
        Link &operator=(const Link &other) = delete;
        Link &operator=(Link &&other) = delete;
        ~Link() = default;

        /**
         * Read the cost value associated to the current link, assuming the given cost family and cost metric.
         *
         * @return 0 if no cost value is explicitly set for the given cost family and/or cost metric.
         *         FIXME It would be probably better to either throw an exception or return a +inf value
         *         (i.e. considering that the underlying link is effectively impassable) if no cost value
         *         is explicitly set. Still, 0 is returned to match the legacy behavior.
         */
        [[nodiscard]] double cost(const std::string &costFamily, const std::string &costMetric) const {
            auto it1 = mcosts.find(costFamily);
            if (it1 == mcosts.end()) {
                return 0;
            }
            auto it2 = it1->second.find(costMetric);
            if (it2 == it1->second.end()) {
                return 0;
            }
            return it2->second;
        }

        void updateCosts(mapcosts costs) {
            mcosts = std::move(costs);
        }
    };


    class Node {
    public:
        std::string mid;
        std::array<double, 2> mposition;
        std::unordered_map<std::string, Link* > madj;
        std::unordered_map<std::string, Link* > mradj;
        std::string mlabel;
        mapsets mexclude_movements;

    private:

        Node(std::string id, double x, double y, std::string label = "", mapsets exclude_movements = {}) :
            mid(std::move(id)),
            mposition{ x, y },
            mlabel(std::move(label)),
            mexclude_movements(std::move(exclude_movements))
        {}

        friend class OrientedGraph;

    public:

        // Relationships between Node and Link instances are materialized through raw pointers.
        // Copying and/or moving these instances would break these relationships.
        Node(const Node &other) = delete;
        Node(Node &&other) = delete;
        Node &operator=(const Node &other) = delete;
        Node &operator=(Node &&other) = delete;
        ~Node() = default;

        /**
         * Assuming we follow a path in the graph, and that we come from node `predecessor`,
         * this method computes all the allowed exit links from the current node, and invokes
         * the given callback for each allowed exit link.
         *
         * The allowed exit links are always a subset of the outgoing links of the current node.
         * Still some outgoing links might be disallowed due to movement restrictions.
         *
         * @param predecessor ID of the previous node on a path.
         * @param callback Must be a callable object with the following signature: `void(const Link *link)`.
         *                 The callback must not modify the graph structure.
         */
        template<typename Callback>
        void forEachExit(const std::string &predecessor, Callback &&callback) const {
            for (const auto &l : madj) {
                auto it = mexclude_movements.find(predecessor);
                if (it == mexclude_movements.end() || it->second.find(l.second->mdown->mid) == it->second.end()) {
                    callback(const_cast<const Link*>(l.second));
                }
            }
        }

        /**
         * Assuming we follow a path in the graph, and that we come from node `predecessor`,
         * this method returns all the allowed exit links from the current node.
         *
         * The allowed exit links are always a subset of the outgoing links of the current node.
         * Still some outgoing links might be disallowed due to movement restrictions.
         *
         * Time-critical functions should prefer using forEachExit() instead (less memory allocation overhead).
         *
         * @param predecessor ID of the previous node on a path.
         */
        [[nodiscard]] std::vector<const Link*> getExits(const std::string &predecessor) const {
            std::vector<const Link*> result;
            forEachExit(predecessor, [&result](const Link *link) { result.emplace_back(link); });
            return result;
        }

    };


    class OrientedGraph {

        std::unordered_map<std::string, Node* > nodes_;
        std::unordered_map<std::string, Link* > links_;

        template<typename Element>
        class ElementRange;

        template<typename Element> // Element is `Node`, `const Node`, `Link` or `const Link`.
        class ElementIterator {

            using Container = std::unordered_map<std::string, std::remove_const_t<Element> *>;

            typename Container::const_iterator it_;

            explicit ElementIterator(typename Container::const_iterator it) : it_(it) {}

            friend class ElementRange<Element>;
            template<typename> friend class ElementIterator;

        public:

            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Element *;
            using pointer = value_type const *;
            using reference = value_type const &;

            ElementIterator() = default;

            /**
             * Implicit conversion from the read-write iterator to the read-only one.
             */
            template<
                typename Other,
                typename = std::enable_if_t<std::is_same_v<const Other, Element> && !std::is_same_v<Other, Element>>
            >
            ElementIterator(const ElementIterator<Other> &other) : it_(other.it_) {}

            [[nodiscard]] pointer operator->() const { return &it_->second; }
            [[nodiscard]] reference operator*() const { return it_->second; }

            ElementIterator &operator++() {
                ++it_;
                return *this;
            }

            ElementIterator operator++(int) {
                ElementIterator result = *this;
                ++it_;
                return result;
            }

            [[nodiscard]] friend bool operator==(const ElementIterator &lhs, const ElementIterator &rhs) {
                return lhs.it_ == rhs.it_;
            }

            [[nodiscard]] friend bool operator!=(const ElementIterator &lhs, const ElementIterator &rhs) {
                return lhs.it_ != rhs.it_;
            }
        };

        template<typename Element> // Element is `Node`, `const Node`, `Link` or `const Link`.
        class ElementRange {

            using Container = std::unordered_map<std::string, std::remove_const_t<Element> *>;

            const Container *container_;

            explicit ElementRange(const Container &container) : container_(&container) {}

            friend class OrientedGraph;
            template<typename> friend class ElementRange;

        public:

            /**
             * Implicit conversion from the read-write range to the read-only one.
             */
            template<
                typename Other,
                typename = std::enable_if_t<std::is_same_v<const Other, Element> && !std::is_same_v<Other, Element>>
            >
            ElementRange(const ElementRange<Other> &other) : container_(other.container_) {}

            [[nodiscard]] ElementIterator<Element> begin() const {
                return ElementIterator<Element>(container_->begin());
            }

            [[nodiscard]] ElementIterator<Element> end() const {
                return ElementIterator<Element>(container_->end());
            }
        };

    public:

        using ConstNodeIterator = ElementIterator<const Node>;
        using ConstNodeRange = ElementRange<const Node>;
        using NodeIterator = ElementIterator<Node>;
        using NodeRange = ElementRange<Node>;
        using ConstLinkIterator = ElementIterator<const Link>;
        using ConstLinkRange = ElementRange<const Link>;
        using LinkIterator = ElementIterator<Link>;
        using LinkRange = ElementRange<Link>;

        /**
         * Number of nodes in the graph.
         */
        [[nodiscard]] std::size_t nodeCount() const {
            return nodes_.size();
        }

        /**
         * Whether a node corresponding to the given ID exists in the graph.
         */
        [[nodiscard]] bool hasNode(const std::string &nodeId) const {
            return nodes_.find(nodeId) != nodes_.end();
        }

        /**
         * Return the node corresponding to the given ID (read-only version).
         *
         * @return Guaranteed to be non-null.
         * @throws std::invalid_argument if no node corresponds to the given ID.
         */
        [[nodiscard]] const Node *node(const std::string &nodeId) const {
            auto it = nodes_.find(nodeId);
            if (it == nodes_.end()) {
                throw std::invalid_argument(StrCat("[OrientedGraph::node] Unknown node ID [", nodeId, "]."));
            }
            return it->second;
        }

        /**
         * Return the node corresponding to the given ID (read-write version).
         *
         * @return Guaranteed to be non-null.
         * @throws std::invalid_argument if no node corresponds to the given ID.
         */
        [[nodiscard]] Node *node(const std::string &nodeId) {

            // Casting away const is safe here: the node is owned by this graph, which is itself
            // non-const in this method, so the node is not actually a const object.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<Node *>(const_cast<const OrientedGraph *>(this)->node(nodeId));
        }

        /**
         * Return the node corresponding to the given ID if it exists (read-only version).
         *
         * @return nullptr if the node does not exist.
         */
        [[nodiscard]] const Node *findNode(const std::string &nodeId) const {
            auto it = nodes_.find(nodeId);
            return it == nodes_.end() ? nullptr : it->second;
        }

        /**
         * Return the node corresponding to the given ID if it exists (read-write version).
         *
         * @return nullptr if the node does not exist.
         */
        [[nodiscard]] Node *findNode(const std::string &nodeId) {

            // Casting away const is safe here: the node is owned by this graph, which is itself
            // non-const in this method, so the node is not actually a const object.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<Node *>(const_cast<const OrientedGraph *>(this)->findNode(nodeId));
        }

        /**
         * Lightweight view of the nodes of the current graph, enumerated in no particular order (read-only version).
         *
         * Do not add or remove nodes on the graph while iterating on the range.
         */
        [[nodiscard]] ConstNodeRange nodes() const {
            return ConstNodeRange(nodes_);
        }

        /**
         * Lightweight view of the nodes of the current graph, enumerated in no particular order (read-write version).
         *
         * Do not add or remove nodes on the graph while iterating on the range.
         */
        [[nodiscard]] NodeRange nodes() {
            return NodeRange(nodes_);
        }

        /**
         * Number of links in the graph.
         */
        [[nodiscard]] std::size_t linkCount() const {
            return links_.size();
        }

        /**
         * Whether a link corresponding to the given ID exists in the graph.
         */
        [[nodiscard]] bool hasLink(const std::string &linkId) const {
            return links_.find(linkId) != links_.end();
        }

        /**
         * Return the link corresponding to the given ID (read-only version).
         *
         * @return Guaranteed to be non-null.
         * @throws std::invalid_argument if no link corresponds to the given ID.
         */
        [[nodiscard]] const Link *link(const std::string &linkId) const {
            auto it = links_.find(linkId);
            if (it == links_.end()) {
                throw std::invalid_argument(StrCat("[OrientedGraph::link] Unknown link ID [", linkId, "]."));
            }
            return it->second;
        }

        /**
         * Return the link corresponding to the given ID (read-write version).
         *
         * @return Guaranteed to be non-null.
         * @throws std::invalid_argument if no link corresponds to the given ID.
         */
        [[nodiscard]] Link *link(const std::string &linkId) {

            // Casting away const is safe here: the link is owned by this graph, which is itself
            // non-const in this method, so the link is not actually a const object.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<Link *>(const_cast<const OrientedGraph *>(this)->link(linkId));
        }

        /**
         * Return the link corresponding to the given ID if it exists (read-only version).
         *
         * @return nullptr if the link does not exist.
         */
        [[nodiscard]] const Link *findLink(const std::string &linkId) const {
            auto it = links_.find(linkId);
            return it == links_.end() ? nullptr : it->second;
        }

        /**
         * Return the link corresponding to the given ID if it exists (read-write version).
         *
         * @return nullptr if the link does not exist.
         */
        [[nodiscard]] Link *findLink(const std::string &linkId) {

            // Casting away const is safe here: the link is owned by this graph, which is itself
            // non-const in this method, so the link is not actually a const object.
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
            return const_cast<Link *>(const_cast<const OrientedGraph *>(this)->findLink(linkId));
        }

        /**
         * Lightweight view of the links of the current graph, enumerated in no particular order (read-only version).
         *
         * Do not add or remove links on the graph while iterating on the range.
         */
        [[nodiscard]] ConstLinkRange links() const {
            return ConstLinkRange(links_);
        }

        /**
         * Lightweight view of the links of the current graph, enumerated in no particular order (read-write version).
         *
         * Do not add or remove links on the graph while iterating on the range.
         */
        [[nodiscard]] LinkRange links() {
            return LinkRange(links_);
        }

        /**
         * Add all the nodes and links from `graph` to the current OrientedGraph.
         *
         * The pre-existing nodes and links of the current OrientedGraph are preserved.
         */
        void AddAllNodesAndLinks(const OrientedGraph &graph);

        void AddNode(std::string id, double x, double y, std::string label = "", mapsets excludeMovements = {});
        void AddLink(std::string id, const std::string &up, const std::string &down, double length, mapcosts costs, std::string label = "");
        void DeleteLink(const std::string &linkId);
        void DeleteAllLinksToNode(const std::string &nodeId);
        void UpdateLinkCosts(const std::string &linkId, mapcosts costs);
        void UpdateCosts(const std::unordered_map<std::string, mapcosts> &linkIdToCosts);
        double getLength(const std::string &up, const std::string &down);
        std::vector<std::string> GetLinksWithoutCost(const std::string &costMetric, const std::unordered_map<std::string, std::string> &labelToCostFamily);

        void ShowNodes();
        void ShowLinks();

        OrientedGraph() = default;
        OrientedGraph(const OrientedGraph &other);
        OrientedGraph(OrientedGraph &&other) noexcept = default;
        OrientedGraph &operator=(const OrientedGraph &other);
        OrientedGraph &operator=(OrientedGraph &&other) noexcept = default;
        ~OrientedGraph();

    };

    OrientedGraph* mergeOrientedGraph(const std::vector<const OrientedGraph*> &graphs);

} // namespace hipop
