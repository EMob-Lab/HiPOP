#pragma once

#include <hipop/graph.h>
#include <hipop/graph_path_cost.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>


using pathCost = std::pair<std::vector<std::string>, double>;
using ShortestPathsTree = std::unordered_map<std::string, std::string>;

namespace hipop {


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
     * @return The list of Nodes defining the shortest path and the associated cost.
     */
    template<typename CostFunction>
    pathCost dijkstraGeneric(
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

        const Node *origin_node = G.mnodes.at(origin);
        const Node *destination_node = G.mnodes.at(destination);

        // Return a 0-node path with zero cost if origin and destination are identical.
        // Remark: it would be more consistent with the general case to return 1-node path
        // made of the single origin + destination node.
        // Still, for compliance with legacy behavior, we return a 0-node path.
        if (origin_node == destination_node) {
            pathCost empty_path;
            empty_path.second = 0;
            return empty_path;
        }

        std::unordered_map<const Node*, double> dist; // No nullptr keys in this map.
        std::unordered_map<const Node*, const Node*> prev; // No nullptr (neither as key nor as value) in this map.
        dist.reserve(G.mnodes.size());
        prev.reserve(G.mnodes.size());
        dist[origin_node] = 0;
        // `prev[origin_node]` is intentionally left unset (the origin node has no predecessor).

        std::priority_queue<DijkstraQueueItem> pq;
        pq.emplace(DijkstraQueueItem{ 0, origin_node });

        while (!pq.empty()) {

            const Node *u = pq.top().node;
            double dist_u = dist.at(u);
            pq.pop();

            if (u == destination_node) {
                pathCost path;
                for (auto it = prev.find(u); it != prev.end(); it = prev.find(it->second)) {
                    path.first.emplace_back(it->first->mid);
                }
                path.first.emplace_back(origin_node->mid);
                std::reverse(path.first.begin(), path.first.end());
                path.second = dist_u;
                return path;
            }

            u->forEachExit(u == origin_node ? "" : prev.at(u)->mid, [&](const Link *link) {
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
                        prev[neighbor] = u;
                    }
                }
            });
        }

        // No path from origin to destination was found: return a 0-node path with infinite cost in this case.
        // Remark: to make it more expicit, it would be better to return an optional<pathCost> instead.
        pathCost invalid_path;
        invalid_path.second = INFINITY;
        return invalid_path;
    }


    /**
     * Compute the shortest path between origin and destination using the Dijkstra algorithm.
     *
     * @param costMetric Cost metric to consider.
     * @param labelToCostFamily Cost family to use for each link label.
     * @param accessibleLinkLabels If non-empty, only the links whose label is in this set
     *                             are considered for the path search.
     * @return The list of Nodes defining the shortest path and the associated cost.
     */
    inline pathCost dijkstra(
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


    ShortestPathsTree dijkstraSingleSource(
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
    pathCost aStar(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels,
        const std::function<double(const Node *, const Node *)> &heuristic);
    pathCost aStarEuclidianDist(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels);

    std::vector<pathCost> parallelDijkstra(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<ShortestPathsTree> parallelDijkstraSingleSource(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<pathCost> parallelDijkstraHeterogeneousCosts(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::vector<std::string> &costMetrics,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels = {});

    std::vector<pathCost> YenKShortestPath(
        OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const setstring &accessibleLinkLabels,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int kPath);
    std::vector<pathCost> KShortestPath(
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

    std::vector<std::vector<pathCost>> parallelKShortestPath(
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

    std::vector<std::vector<pathCost>> parallelKIntermodalShortestPath(
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
