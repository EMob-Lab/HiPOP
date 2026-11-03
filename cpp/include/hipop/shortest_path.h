#pragma once

#include <hipop/graph.h>
#include <hipop/graph_path.h>
#include <hipop/graph_path_cost.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <queue>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


namespace hipop {


    /**
     * Helper alias for (graph-path, cost-value).
     *
     * Most path search functions use it as their return type.
     */
    using PathCost = std::pair<OrientedGraphPath, double>;


    /**
     * Element of the priority queue used in the Dijkstra/A* algorithms.
     *
     * Do NOT use in non-HiPOP code.
     */
    struct DijkstraQueueItem {

        double dist;
        const Node *node;

        /**
         * Comparison operator implemented so that:
         * - the top item in the queue is the one with the smallest dist,
         * - ... or, in case of ties, the one whose ID comes first in lexicographical order.
         */
        bool operator<(const DijkstraQueueItem &other) const {
            return dist == other.dist ? node->mid > other.node->mid : dist > other.dist;
        }
    };


    /**
     * Compute the shortest path between origin and destination using the Dijkstra algorithm.
     *
     * How a cost value is assigned to each link is defined by the given cost function.
     *
     * @param costFunction Must be a callable object with the following signature: `double(const Link *link)`.
     *                     Should be a pure function (i.e. no side effects).
     * @param accessibleLinkLabels If non-empty, only the links whose label is in this set
     *                             are considered for the path search.
     * @return Shortest path + the associated cost, or std::nullopt if no path exist from origin to destination.
     */
    template<typename CostFunction>
    std::optional<PathCost> dijkstraGeneric(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const CostFunction &costFunction,
        const setstring &accessibleLinkLabels
    ) {
        static_assert(
            std::is_invocable_r_v<double, CostFunction, const Link *>,
            "Cost function must have signature: double(const Link *link)"
        );

        const Node *origin_node = G.node(origin);
        const Node *destination_node = G.node(destination);

        // Return an empty path (i.e. no link) with zero cost if origin and destination are identical.
        if (origin_node == destination_node) {
            return PathCost{ OrientedGraphPath(origin_node), 0 };
        }

        // `dist[n]` is the best cost achieved so far to reach node n from origin.
        // `prev[n]` is the last link of the best path achieved so far from origin to node n.
        // No null pointers in these maps (neither as keys nor as values).
        std::unordered_map<const Node*, double> dist;
        std::unordered_map<const Node*, const Link*> prev;
        dist.reserve(G.nodeCount());
        prev.reserve(G.nodeCount());
        dist[origin_node] = 0;

        std::priority_queue<DijkstraQueueItem> pq;
        pq.emplace(DijkstraQueueItem{ 0, origin_node });

        while (!pq.empty()) {

            const Node *u = pq.top().node;
            double dist_u = dist.at(u);
            pq.pop();

            if (u == destination_node) {
                std::vector<const Link*> links;
                const Node *v = destination_node;
                while (v != origin_node) {
                    const Link *link_to_v = prev.at(v);
                    links.emplace_back(link_to_v);
                    v = link_to_v->mup;
                }
                std::reverse(links.begin(), links.end());
                return PathCost{ OrientedGraphPath(origin_node, std::move(links)), dist_u };
            }

            u->forEachExit(u == origin_node ? "" : prev.at(u)->mup->mid, [&](const Link *link) {
                if (accessibleLinkLabels.empty() || accessibleLinkLabels.find(link->mlabel) != accessibleLinkLabels.end()) {

                    // The Dijkstra algorithm requires that all link costs are >= 0, and so does
                    // the current implementation. `cost_on_link` must NOT be NaN either.
                    // However, having link costs equal to +infinity is allowed:
                    // the corresponding links are never visited.

                    double cost_on_link = costFunction(link);
                    double new_dist = dist_u + cost_on_link;
                    const Node *neighbor = link->mdown;

                    auto neighbor_it = dist.try_emplace(neighbor, INFINITY).first;
                    if (neighbor_it->second > new_dist) { // Follow the link only if it STRICTLY improves the distance.
                        neighbor_it->second = new_dist;
                        pq.emplace(DijkstraQueueItem{ new_dist, neighbor });
                        prev[neighbor] = link;
                    }
                }
            });
        }
        return std::nullopt; // No path exists from origin to destination.
    }


    /**
     * Compute the shortest path between origin and destination using the Dijkstra algorithm.
     *
     * @param costMetric Cost metric to consider.
     * @param labelToCostFamily Cost family to use for each link label.
     * @param accessibleLinkLabels If non-empty, only the links whose label is in this set
     *                             are considered for the path search.
     * @return Shortest path + the associated cost, or std::nullopt if no path exist from origin to destination.
     */
    inline std::optional<PathCost> dijkstra(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels = {}
    ) {
        LabelToCostFamilyLinkCostFunction costFunction(labelToCostFamily, costMetric);
        return dijkstraGeneric(G, origin, destination, costFunction, accessibleLinkLabels);
    }


    /**
     * Container for the paths returned by dijkstraSingleSource.
     *
     * All the paths in this tree share the same origin node.
     */
    class SameOriginPathTree {

        // The pointers in this class a never null (neither for `origin_`
        // nor for the keys/values in `destinationToLastLink_`).
        const Node *origin_;
        std::unordered_map<const Node *, const Link *> destinationToLastLink_;

        explicit SameOriginPathTree(const Node *origin) : origin_(origin) {}

        friend SameOriginPathTree dijkstraSingleSource(
            const OrientedGraph &,
            const std::string &,
            const std::string &,
            const std::unordered_map<std::string, std::string> &,
            const setstring &
        );

    public:

        /**
         * Origin - aka. first node - of all the paths in the current tree.
         */
        [[nodiscard]] const Node *origin() const {
            return origin_;
        }

        /**
         * Whether or not a path from the origin to the given destination node exists in the current tree.
         */
        bool hasPathTo(const Node *destination) const {
            return destination == origin_ || destinationToLastLink_.find(destination) != destinationToLastLink_.end();
        }

        class DestinationRange;

        class DestinationIterator {

            using ContainerIterator = std::unordered_map<const Node *, const Link *>::const_iterator;

            const Node *origin_ = nullptr; // Non-null iff the iterator currently points to the origin node.
            ContainerIterator it_;

            DestinationIterator(const Node *origin, ContainerIterator it) : origin_(origin), it_(it) {}

            friend class DestinationRange;

        public:

            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const Node *;
            using pointer = void;
            using reference = value_type;

            DestinationIterator() = default;

            [[nodiscard]] reference operator*() const {
                return origin_ != nullptr ? origin_ : it_->first;
            }

            DestinationIterator &operator++() {
                if (origin_ != nullptr) {
                    origin_ = nullptr;
                }
                else {
                    ++it_;
                }
                return *this;
            }

            DestinationIterator operator++(int) {
                DestinationIterator previous = *this;
                operator++();
                return previous;
            }

            [[nodiscard]] friend bool operator==(const DestinationIterator &lhs, const DestinationIterator &rhs) {
                return lhs.origin_ == rhs.origin_ && lhs.it_ == rhs.it_;
            }

            [[nodiscard]] friend bool operator!=(const DestinationIterator &lhs, const DestinationIterator &rhs) {
                return !(lhs == rhs);
            }
        };

        /**
         * Lightweight view over the destination nodes of a tree, to be used in range-based for-loops.
         */
        class DestinationRange {

            const SameOriginPathTree *tree_;

            explicit DestinationRange(const SameOriginPathTree *tree) : tree_{ tree } {}

            friend class SameOriginPathTree;

        public:

            using iterator = DestinationIterator;

            [[nodiscard]] bool empty() const {
                return false; // There is always at least 1 destination: the origin node.
            }

            [[nodiscard]] std::size_t size() const {
                return tree_->destinationToLastLink_.size() + 1;
            }

            [[nodiscard]] iterator begin() const {
                return { tree_->origin_, tree_->destinationToLastLink_.begin() };
            }

            [[nodiscard]] iterator end() const {
                return { nullptr, tree_->destinationToLastLink_.end() };
            }
        };

        /**
         * Destination nodes reachable from the origin in the current tree (including the origin itself).
         */
        [[nodiscard]] DestinationRange destinations() const {
            return DestinationRange{ this };
        }

        /**
         * Return the path from the origin to the given destination node, if it exists in the current tree.
         *
         * @throws std::invalid_argument if no path to the given destination node exists in the current tree.
         */
        [[nodiscard]] OrientedGraphPath pathTo(const Node *destination) const {

            if (!hasPathTo(destination)) {
                throw std::invalid_argument(
                    StrCat("[SameOriginPathTree::pathTo] No path to node=[", destination->mid, "].")
                );
            }

            std::vector<const Link *> links;
            const Node *currentNode = destination;
            while (currentNode != origin_) {
                const Link *link = destinationToLastLink_.at(currentNode);
                links.emplace_back(link);
                currentNode = link->mup;
            }
            std::reverse(links.begin(), links.end());
            return { origin_, std::move(links) };
        }

        /**
         * Return the path from the origin to the given destination node, if it exists in the current tree.
         *
         * @return std::nullopt if no path to the given destination node exists in the current tree.
         */
        [[nodiscard]] std::optional<OrientedGraphPath> findPathTo(const Node *destination) const {
            return hasPathTo(destination) ? std::make_optional(pathTo(destination)) : std::nullopt;
        }

    };


    SameOriginPathTree dijkstraSingleSource(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels);
    std::pair<std::vector<std::vector<int>>, std::unordered_map<int, std::string>> floydWarshall(
        const OrientedGraph &G,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels);
    std::optional<PathCost> aStar(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels,
        const std::function<double(const Node *, const Node *)> &heuristic);
    std::optional<PathCost> aStarEuclidianDist(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels);

    std::vector<std::optional<PathCost>> parallelDijkstra(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<SameOriginPathTree> parallelDijkstraSingleSource(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<std::optional<PathCost>> parallelDijkstraHeterogeneousCosts(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::vector<std::string> &costMetrics,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<PathCost> YenKShortestPath(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const setstring &accessibleLinkLabels,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int kPath);
    std::vector<PathCost> KShortestPath(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const setstring &accessibleLinkLabels,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        double maxDiffCost,
        double maxDistInCommon,
        double costMultiplier,
        int maxRetry,
        int kPath,
        bool intermodal);

    std::vector<std::vector<PathCost>> parallelKShortestPath(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::string &costMetric,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::vector<setstring> &accessibleLinkLabels,
        double maxDiffCost,
        double maxDistInCommon,
        double costMultiplier,
        int maxRetry,
        const std::vector<int> &kPaths,
        int threadNumber);

    std::vector<std::vector<PathCost>> parallelKIntermodalShortestPath(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::pair<std::unordered_set<std::string>, std::unordered_set<std::string>> &pairMandatoryLabels,
        double maxDiffCost,
        double maxDistInCommon,
        double costMultiplier,
        int maxRetry,
        const std::vector<int> &kPaths,
        const std::vector<setstring> &accessibleLinkLabels = {});

} // namespace hipop
