#include <hipop/shortest_path.h>

#include <hipop/string_util.h>

#include <omp.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <unordered_set>


namespace { // Anonymous namespace

    /**
     * Suffix codes added to the node IDs when building an intermodal graph.
     *
     * No suffix code may end with another suffix code (e.g. "_BB" would be incompatible with "_AA_BB"),
     * otherwise the suffix removal logic implemented in removeIntermodalNodeSuffix(..) would fail.
     *
     * The "_N_" prefix (for "node") ensures that no node suffix ends with a link suffix, or the other way around.
     * The path search algorithms do not need this non-overlap property, but it makes some bugs easier to spot,
     * e.g. calling removeIntermodalNodeSuffix(..) where removeIntermodalLinkSuffix(..) was intended, or vice versa.
     */
    namespace IntermodalNodeSuffix {
        constexpr std::string_view ORIGINAL = "_N_ORIG";
        constexpr std::string_view TWIN = "_N_TWIN";
        constexpr std::string_view TRIPLE = "_N_TRPL";
    }


    /**
     * Suffix codes added to the link IDs when building an intermodal graph.
     *
     * No suffix code may end with another suffix code (e.g. "_TWIN" would be incompatible with "_ORIG_TWIN"),
     * otherwise the suffix removal logic implemented in removeIntermodalLinkSuffix(..) would fail.
     */
    namespace IntermodalLinkSuffix {
        constexpr std::string_view ORGINAL_ORIGINAL = "_ORIG_ORIG"; // Between original nodes
        constexpr std::string_view TWIN_TWIN = "_TWIN_TWIN";        // Between twin nodes
        constexpr std::string_view TRIPLE_TRIPLE = "_TRPL_TRPL";    // Between triple nodes
        constexpr std::string_view ORIGINAL_TWIN = "_ORIG_TWIN";    // From an original node to a twin node
        constexpr std::string_view TWIN_TRIPLE = "_TWIN_TRPL";      // From a twin node to a triple node
    }


    /**
     * Remove the intermodal suffix (one of {@link IntermodalNodeSuffix}) from the given node ID.
     *
     * @param nodeId - Must be a node ID from an intermodal graph (i.e. it must end with one of the searched suffixes).
     *                 Otherwise, an exception is thrown.
     */
    std::string removeIntermodalNodeSuffix(std::string_view nodeId) {
        std::size_t sz = nodeId.size();
        for (const std::string_view &suffix : {
            IntermodalNodeSuffix::ORIGINAL,
            IntermodalNodeSuffix::TWIN,
            IntermodalNodeSuffix::TRIPLE,
        }) {
            if (sz >= suffix.size() && nodeId.substr(sz - suffix.size()) == suffix) {
                return static_cast<std::string>(nodeId.substr(0, sz - suffix.size()));
            }
        }
        throw std::invalid_argument(
            hipop::StrCat("[removeIntermodalNodeSuffix] No intermodal suffix found in node ID [", nodeId, "].")
        );
    }


    /**
     * Remove the intermodal suffix (one of {@link IntermodalLinkSuffix}) from the given link ID.
     *
     * @param linkId - Must be a link ID from an intermodal graph (i.e. it must end with one of the searched suffixes).
     *                 Otherwise, an exception is thrown.
     */
    std::string removeIntermodalLinkSuffix(std::string_view linkId) {
        std::size_t sz = linkId.size();
        for (const std::string_view &suffix : {
            IntermodalLinkSuffix::ORGINAL_ORIGINAL,
            IntermodalLinkSuffix::TWIN_TWIN,
            IntermodalLinkSuffix::TRIPLE_TRIPLE,
            IntermodalLinkSuffix::ORIGINAL_TWIN,
            IntermodalLinkSuffix::TWIN_TRIPLE,
        }) {
            if (sz >= suffix.size() && linkId.substr(sz - suffix.size()) == suffix) {
                return static_cast<std::string>(linkId.substr(0, sz - suffix.size()));
            }
        }
        throw std::invalid_argument(
            hipop::StrCat("[removeIntermodalLinkSuffix] No intermodal suffix found in link ID [", linkId, "].")
        );
    }


}


namespace hipop
{

    /**
     * @brief Compute the shortest paths between origin and all other vertices of the
     *        graph using the Dijkstra algorithm
     *
     * @param G The OrientedGrah used for the shortest path
     * @param origin The origin
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param labelToCostFamily The cost family to use for each link label (multiple cost families can be defined on a Link)
     * @param accessibleLinkLabels The set of accessible link labels
     * @return prev The shortest paths tree from origin
     */
    ShortestPathsTree dijkstraSingleSource(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels)
    {
        const Node *origin_node = G.mnodes.at(origin);

        std::unordered_map<const Node*, double> dist; // No nullptr keys in this map.
        ShortestPathsTree prev; // Values equals to "" (empty string) means "no predecessor" in this map.
        dist.reserve(G.mnodes.size());
        prev.reserve(G.mnodes.size());
        for (const auto &it : G.mnodes) {
            dist.emplace(it.second, origin_node == it.second ? 0 : INFINITY);
            prev.emplace(it.second->mid, "");
        }

        std::priority_queue<DijkstraQueueItem> pq;
        pq.emplace(DijkstraQueueItem{ 0, origin_node });

        while (!pq.empty()) {

            const Node *u = pq.top().node;
            double dist_u = dist.at(u);
            pq.pop();

            u->forEachExit(prev.at(u->mid), [&](const Link *link) {
                if (accessibleLinkLabels.empty() || accessibleLinkLabels.find(link->mlabel) != accessibleLinkLabels.end()) {

                    // The Dijkstra algorithm requires that all link costs are >= 0, and so does
                    // the current implementation. `cost_on_link` must NOT be NaN either.
                    // However, having link costs equal to +infinity is allowed:
                    // the corresponding links are never visited.

                    double cost_on_link = link->cost(labelToCostFamily.at(link->mlabel), costMetric);
                    double new_dist = dist_u + cost_on_link;
                    const Node *neighbor = link->mdown;

                    auto neighbor_it = dist.find(neighbor);
                    if (neighbor_it->second > new_dist) { // Follow the link only if it STRICTLY improves the distance.
                        neighbor_it->second = new_dist;
                        pq.emplace(DijkstraQueueItem{ new_dist, neighbor });
                        prev[neighbor->mid] = u->mid;
                    }
                }
            });
        }
        return prev;
    }

    /**
     * @brief Compute the shortest paths between all pairs of vertices
     *
     * @param G The OrientedGrah
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param labelToCostFamily The cost family to use for each link label (multiple cost families can be defined on a Link)
     * @param accessibleLinkLabels The set of accessible link labels
     * @return prev The shortest paths tree
     */
    std::pair<std::vector<std::vector<int>>, std::unordered_map<int, std::string>> floydWarshall(
        const OrientedGraph &G,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels)
    {
        int V = G.mnodes.size();

        // Map nodes ids with integers
        std::unordered_map<std::string, int> nodevMap;
        std::unordered_map<int, std::string> vnodeMap;
        {
            int v = 0;
            for (const auto& pair : G.mnodes)
            {
                nodevMap.insert({pair.first, v});
                vnodeMap.insert({v, pair.first});
                ++v;
            }
        }

        // Initialize dist and prev tables
        double inf = std::numeric_limits<double>::infinity();
        int null = V+1;
        std::vector<std::vector<double>> dist(V, std::vector<double>(V, inf));
        std::vector<std::vector<int>> prev(V, std::vector<int>(V, null));
        for (const auto& pair : G.mlinks)
        {
            Link*link = pair.second;
            if (accessibleLinkLabels.empty() || accessibleLinkLabels.find(link->mlabel) != accessibleLinkLabels.end())
            {
                const std::string &u = link->mup->mid;
                const std::string &v = link->mdown->mid;
                dist[nodevMap.at(u)][nodevMap.at(v)] = link->cost(labelToCostFamily.at(link->mlabel), costMetric);
                prev[nodevMap.at(u)][nodevMap.at(v)] = nodevMap.at(u);
            }
        }
        for (int v = 0; v < V; ++v)
        {
            dist[v][v] = 0;
            prev[v][v] = v;
        }

        // Proceed
        for (int k = 0; k < V; ++k)
        {
            for (int i = 0; i < V; ++i)
            {
                for (int j = 0; j < V; ++j)
                {
                    if (dist[i][k] != inf && dist[k][j] != inf && dist[i][j] > dist[i][k] + dist[k][j])
                    {
                        dist[i][j] = dist[i][k] + dist[k][j];
                        prev[i][j] = prev[k][j];
                    }
                }
            }
        }

        return std::make_pair(prev, vnodeMap);

    }

    /**
     * @brief Find duplicated origin - destination - labelToCostFamily - costMetric to prevent
     *        computing several times the same shortest paths
     *
     * @param origins The vector of origins
     * @param destinations The vector of destinations
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param costMetrics The vector of cost metrics to consider in the shortest path algorithm
     * @return r The tuple with a vector of unique elements indices
     *         as first element,  a map of correspondance between non unique index and the unique
     *         index with the same origin - destination - labelToCostFamily as second element,
     *         and a map of correspondance between unique index and the number of
     *         shortest paths to compute for it
     */
    std::tuple<std::vector<int>, std::unordered_map<int, int>, std::unordered_map<int, int>> find_duplicates(
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::vector<std::string> & costMetrics,
        const std::vector<int> &kPaths)
    {
        int nbODs = origins.size();
        std::vector<std::string> ODsLabelCosts;
        for (int i = 0; i < nbODs; ++i)
        {
            const std::string &o = origins[i];
            const std::string &d = destinations[i];
            const std::string &costMetric = costMetrics[i];
            const std::unordered_map<std::string, std::string> &currentLabelToCostFamily = labelToCostFamily[i];
            std::vector<std::pair<std::string, std::string>> vecLabelCosts = std::vector<std::pair<std::string, std::string>>(currentLabelToCostFamily.begin(), currentLabelToCostFamily.end());
            std::sort(vecLabelCosts.begin(), vecLabelCosts.end(), [](const std::pair<std::string,std::string> &left, const std::pair<std::string,std::string> &right) {
                return left.first < right.first;
            });

            std::string ODLabelCosts = StrCat(o, d, costMetric);
            for (const auto &labelCosts : vecLabelCosts)
            {
                StrAppend(ODLabelCosts, "-", labelCosts.first, ":", labelCosts.second);
            }
            ODsLabelCosts.emplace_back(std::move(ODLabelCosts));
        }

        std::set<std::string> s;
        std::vector<int> uniqueIndices;
        std::unordered_map<int, int> nbPaths;
        std::unordered_map<int, int> duplicateIndices;
        for (std::size_t i = 0; i < ODsLabelCosts.size(); i++)
        {
            if (s.insert(ODsLabelCosts[i]).second)
            {
                uniqueIndices.push_back(i);
                if (!kPaths.empty())
                {
                    nbPaths[i] = kPaths[i];
                }
                else
                {
                    nbPaths[i] = 1;
                }

            } else
            {
                 int corresponding_uniqueIdx= find(ODsLabelCosts.begin(), ODsLabelCosts.end(), ODsLabelCosts[i]) - ODsLabelCosts.begin();
                 duplicateIndices[i] = corresponding_uniqueIdx;
                 if ((!kPaths.empty()) && (kPaths[i] > nbPaths[corresponding_uniqueIdx]))
                 {
                    nbPaths[corresponding_uniqueIdx] = kPaths[i];
                 }
            }
        }

        std::tuple<std::vector<int>, std::unordered_map<int, int>, std::unordered_map<int, int>> r = std::make_tuple(uniqueIndices, duplicateIndices, nbPaths);

        return r;
    }

    /**
     * @brief Batch computation of shortest with openmp using the Dijkstra algorithm
     *
     * @param G The OrientedGrah used for the shortest paths
     * @param origins The vector of origins
     * @param destinations The vector of destinations
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param threadNumber The number of thread for openmp
     * @param accessibleLinkLabels The vector of accessible link labels
     * @return std::vector<pathCost> The vector of computed shortest path
     */
    std::vector<std::optional<PathCost>> parallelDijkstra(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels)
    {
        omp_set_num_threads(threadNumber);

        int nbPath = origins.size();
        std::vector<int> emptyV;
        std::vector<int> uniqueIndices;
        std::vector<std::string> costMetrics(nbPath, costMetric);
        std::unordered_map<int, int> duplicateIndices;
        std::unordered_map<int, int> nbPathsPerOD;
        tie(uniqueIndices, duplicateIndices, nbPathsPerOD) = find_duplicates(origins, destinations, labelToCostFamily, costMetrics, emptyV);

        std::vector<std::optional<PathCost>> res(nbPath);

        // FIXME MSVC is still stuck to OpenMP 2.0, which requires **signed** loop variables for parallel for.
        std::int64_t nbUniqueIndices = uniqueIndices.size();

        #pragma omp parallel for shared(res, accessibleLinkLabels, labelToCostFamily) schedule(dynamic)
        for (std::int64_t i = 0; i < nbUniqueIndices; i++)
        {
            int uniqueIdx = uniqueIndices[i];
            if (accessibleLinkLabels.empty())
            {
                res[uniqueIdx] = dijkstra(G, origins[uniqueIdx], destinations[uniqueIdx], costMetric, labelToCostFamily[uniqueIdx], {});
            }
            else
            {
                res[uniqueIdx] = dijkstra(G, origins[uniqueIdx], destinations[uniqueIdx], costMetric, labelToCostFamily[uniqueIdx], accessibleLinkLabels[uniqueIdx]);
            }
        }

        for (const auto& elem : duplicateIndices)
        {
            res[elem.first] = res[elem.second];
        }

        return res;
    }

    /**
     * @brief Computation of shortest paths from each origin provided to all other
     *        vertices in the graph
     *
     * @param G The OrientedGrah used for the shortest paths
     * @param origins The vector of origins
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param threadNumber The number of thread for openmp
     * @param accessibleLinkLabels The vector of accessible link labels
     * @return std::vector<ShortestPathsTree> The vector of computed shortest paths trees
     */
    std::vector<ShortestPathsTree> parallelDijkstraSingleSource(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::string &costMetric,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels)
    {
        omp_set_num_threads(threadNumber);

        int nbSPTs = origins.size();
        std::vector<ShortestPathsTree> res(nbSPTs);

        #pragma omp parallel for shared(res, accessibleLinkLabels, labelToCostFamily) schedule(dynamic)
        for (int i = 0; i < nbSPTs; i++)
        {
            if (accessibleLinkLabels.empty())
            {
                res[i] = dijkstraSingleSource(G, origins[i], costMetric, labelToCostFamily[i], {});
            }
            else
            {
                res[i] = dijkstraSingleSource(G, origins[i], costMetric, labelToCostFamily[i], accessibleLinkLabels[i]);
            }
        }

        return res;
    }

    /**
     * @brief Batch computation of shortest paths with openmp using the Dijkstra
     *        algorithm and considering different costs for each path
     * @param G The OrientedGrah used for the shortest paths
     * @param origins The vector of origins
     * @param destinations The vector of destinations
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param costMetrics The vector of cost metrics to consider in the shortest path algorithm
     * @param threadNumber The number of thread for openmp
     * @param accessibleLinkLabels The vector of accessible link labels
     * @return std::vector<pathCost> The vector of computed shortest path
     */
    std::vector<std::optional<PathCost>> parallelDijkstraHeterogeneousCosts(
        const OrientedGraph &G,
        const std::vector<std::string> &origins,
        const std::vector<std::string> &destinations,
        const std::vector<std::unordered_map<std::string, std::string>> &labelToCostFamily,
        const std::vector<std::string> &costMetrics,
        int threadNumber,
        const std::vector<setstring> &accessibleLinkLabels)
    {
        omp_set_num_threads(threadNumber);

        std::vector<int> emptyV;
        std::vector<int> uniqueIndices;
        std::unordered_map<int, int> duplicateIndices;
        std::unordered_map<int, int> nbPathsPerOD;
        tie(uniqueIndices, duplicateIndices, nbPathsPerOD) = find_duplicates(origins, destinations, labelToCostFamily, costMetrics, emptyV);

        int nbPath = origins.size();
        std::vector<std::optional<PathCost>> res(nbPath);

        // FIXME MSVC is still stuck to OpenMP 2.0, which requires **signed** loop variables for parallel for.
        std::int64_t nbUniqueIndices = uniqueIndices.size();

        #pragma omp parallel for shared(res, accessibleLinkLabels, labelToCostFamily) schedule(dynamic)
        for (std::int64_t i = 0; i < nbUniqueIndices; i++)
        {
            int uniqueIdx = uniqueIndices[i];
            if (accessibleLinkLabels.empty())
            {
                res[uniqueIdx] = dijkstra(G, origins[uniqueIdx], destinations[uniqueIdx], costMetrics[uniqueIdx], labelToCostFamily[uniqueIdx], {});
            }
            else
            {
                res[uniqueIdx] = dijkstra(G, origins[uniqueIdx], destinations[uniqueIdx], costMetrics[uniqueIdx], labelToCostFamily[uniqueIdx], accessibleLinkLabels[uniqueIdx]);
            }
        }

        for (const auto& elem : duplicateIndices)
        {
            res[elem.first] = res[elem.second];
        }

        return res;
    }


    /**
     * Increase the cost of each link encountered along the given path.
     *
     * @param overridenCosts Container holding the modified costs.
     * @param costMultiplier Multiplier to apply to the links costs of the path.
     */
    void increaseCostsFromPath(
        const OrientedGraphPath &path,
        const LabelToCostFamilyLinkCostFunction &costFunction,
        std::unordered_map<const Link *, double> &overridenCosts,
        double costMultiplier)
    {
        for (const Link *link : path.links()) {

            // Copy the initial cost value for the current link to the `overridenCosts` map
            // if not already present, and apply the cost muliplier to it.
            auto it = overridenCosts.find(link);
            if (it == overridenCosts.end()) {
                double initialCostValue = costFunction(link);
                it = overridenCosts.emplace(link, initialCostValue).first;
            }
            it->second *= costMultiplier;
        }
    }


    /**
     * Increase the cost of each link encountered along the given intermodal path.
     *
     * @param overridenCosts Container holding the modified costs.
     * @param costMultiplier Multiplier to apply to the links costs of the path.
     */
    void increaseCostsFromIntermodalPath(
        const OrientedGraph &G,
        const OrientedGraphPath &path,
        const LabelToCostFamilyLinkCostFunction &costFunction,
        std::unordered_map<const Link *, double> &overridenCosts,
        double costMultiplier)
    {
        for (const Link *visited_link : path.links()) {
            std::string decoded_link_id = removeIntermodalLinkSuffix(visited_link->mid);

            for (std::string_view link_suffix : {
                IntermodalLinkSuffix::ORGINAL_ORIGINAL,
                IntermodalLinkSuffix::TWIN_TWIN,
                IntermodalLinkSuffix::TRIPLE_TRIPLE,
                IntermodalLinkSuffix::ORIGINAL_TWIN,
                IntermodalLinkSuffix::TWIN_TRIPLE,
            }) {
                auto corresponding_link_it = G.mlinks.find(StrCat(decoded_link_id, link_suffix));
                if (corresponding_link_it == G.mlinks.end()) {
                    continue; // The corresponding link may not exist for all suffixes.
                }
                const Link *corresponding_link = corresponding_link_it->second;

                // Copy the initial cost value for the current link to the `overridenCosts` map
                // if not already present, and apply the cost muliplier to it.
                auto it = overridenCosts.find(corresponding_link);
                if (it == overridenCosts.end()) {
                    double initialCostValue = costFunction(corresponding_link);
                    it = overridenCosts.emplace(corresponding_link, initialCostValue).first;
                }
                it->second *= costMultiplier;
            }
        }
    }


    /**
     * Compute the maximum relative distance in common between the new path
     * and the pre-existing paths.
     */
    double computeMaxRelativeDistanceInCommon(
        const OrientedGraphPath &newPath,
        const std::vector<PathCost> &existingPaths)
    {
        double newPathLength = computePathLength(newPath);
        double result = 0;
        for (const PathCost &existingPath : existingPaths) {

            double existingPathLength = computePathLength(existingPath.first);

            std::unordered_set<const Link *> linksInExistingPath;
            linksInExistingPath.insert(existingPath.first.links().begin(), existingPath.first.links().end());

            double commonDist = 0;
            for (const Link *link : newPath.links()) {
                if (linksInExistingPath.find(link) != linksInExistingPath.end()) {
                    commonDist += link->mlength;
                }
            }

            double maxLength = std::fmax(newPathLength, existingPathLength);
            double currentRelativeDist = maxLength == 0 ? INFINITY : commonDist / maxLength;
            result = std::fmax(result, currentRelativeDist);
        }
        return result;
    }


    /**
     * @brief Function that decodes an intermodal path computed on a tripled graph
     *
     * @param path The path to decode
     * @return decodedPath The decoded path
     */
    std::vector<std::string> decodeIntermodalPath(const std::vector<std::string> &path)
    {
        std::vector<std::string> decodedPath;
        decodedPath.reserve(path.size());
        for (const std::string &pathNodeId : path) {
            decodedPath.emplace_back(removeIntermodalNodeSuffix(pathNodeId));
        }
        return decodedPath;
    }

    /**
     * @brief Compute K shortest paths for an origin/destination. The first path is normally computed, then we increase the costs of the first path and perform a Dijkstra again.
     * The last step is performed until either we reach K accepted shortest paths or the last computed shortest path do not meet the requirements in terms of maxDiffCost and maxDistInCommon.
     *
     * @param G The OrientedGraph on which we compute the paths
     * @param origin The origin
     * @param destination The destination
     * @param costMetric The cost metric to consider
     * @param accessibleLinkLabels The set of accessible link labels
     * @param labelToCostFamily The cost family to use for each link label
     * @param maxDiffCost The maximal difference between the cost of the first computed
     *        shortest path and the cost of the next ones, expressed as a percentage
     *        (e.g. 0.1 means that the cost of the next path should be less than 101%
     *        of the cost of the first computed shortest path)
     * @param maxDistInCommon The maximal distance in common between the first shortest
     *        path found and the next ones, expressed as a percentage (e.g. 0.6 means that
     *        the next path should have less than 60% common distance with the first computed
     *        shortest path to be accepted)
     * @param costMultiplier The multiplier applied to the links costs of an
     *        accepted shortest path
     * @param maxRetry Maximum number of times we retry to find an acceptable shorest path
     * @param kPath The number of paths to compute
     * @param intermodal Specifies we search k intermodal shortest paths on a tripled graph
     * @return Vector of length <= kPath (possibly empty), containing the found paths.
     */
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
        bool intermodal)
    {
        if (kPath <= 0) {
            throw std::invalid_argument("[KShortestPath] kPath must be >= 1.");
        }
        if (maxRetry <= 0) {
            throw std::invalid_argument("[KShortestPath] maxRetry must be >= 1.");
        }

        LabelToCostFamilyLinkCostFunction costFunction(labelToCostFamily, costMetric);
        std::vector<PathCost> paths;
        auto firstPath = dijkstraGeneric(G, origin, destination, costFunction, accessibleLinkLabels);

        // Stop here if no path from origin to destination is found.
        if (!firstPath.has_value()) {
            return paths;
        }

        paths.emplace_back(*firstPath);

        // Stop here if:
        // 1) origin == destination, i.e. the first path is empty (no alternative path can be found in that case),
        // 2) only 1 path is requested by the caller (i.e. kPath == 1).
        if (firstPath->first.empty() || kPath == 1) {
            return paths;
        }

        // To compute the k^th path, the K-shortest-path algorithm needs to modified the cost value
        // assigned to some links visited by the previous paths.
        // The modified cost values are stored in map `overridenCosts`,
        // so that the graph data-structure itself remains unmodified by KShortestPath(..).
        std::unordered_map<const Link*, double> overridenCosts;

        // Read the cost value for the given link in `overridenCosts` if present,
        // otherwise fallback to the default cost-resolution heuristic.
        auto overridenCostFunction = [&overridenCosts, &costFunction](const Link *link) {
            auto it = overridenCosts.find(link);
            return it != overridenCosts.end() ? it->second : costFunction(link);
        };

        if (intermodal)
        {
            increaseCostsFromIntermodalPath(G, firstPath->first, costFunction, overridenCosts, costMultiplier);
        }
        else
        {
            increaseCostsFromPath(firstPath->first, costFunction, overridenCosts, costMultiplier);
        }

        int retry = 0;
        while (true) {

            auto newPath = dijkstraGeneric(G, origin, destination, overridenCostFunction, accessibleLinkLabels);
            if (!newPath.has_value()) {
                break;
            }

            // The path cost returned by Dijkstra is computed using the overriden costs,
            // but we need the "true" path cost for the rest of the algorithm
            // (i.e. the one computed based on the unmodified cost values).
            newPath->second = computePathCostGeneric(newPath->first, costFunction);

            // Check conditions to accept this new path
            double distInCommon = computeMaxRelativeDistanceInCommon(newPath->first, paths);
            bool maxDistInCommonChecked = distInCommon <= maxDistInCommon;
            bool maxDiffCostChecked = (newPath->second - firstPath->second) / firstPath->second <= maxDiffCost;
            bool isNew = true;
            if (intermodal)
            {
                // TODO replug isNew = std::all_of(paths.cbegin(), paths.cend(), [&newPath](const pathCost &p){
                //     return decodeIntermodalPath(p.first) != decodeIntermodalPath(newPath->first);
                // });
            }
            else
            {
                isNew = std::all_of(paths.cbegin(), paths.cend(), [&newPath](const PathCost &p) {
                    return p.first != newPath->first;
                });
            }

            if (maxDistInCommonChecked && maxDiffCostChecked && isNew)
            {
                paths.emplace_back(*newPath);
                retry = 0;
            }
            else
            {
                retry += 1;
            }

            if (paths.size() >= static_cast<unsigned int>(kPath) || retry >= maxRetry) {
                break; // The target number of paths is reached, or the maximum number of attempts.
            }

            if (intermodal)
            {
                increaseCostsFromIntermodalPath(G, newPath->first, costFunction, overridenCosts, costMultiplier);
            }
            else
            {
                increaseCostsFromPath(newPath->first, costFunction, overridenCosts, costMultiplier);
            }
        }

        return paths;
    }

    /**
     * @brief Compute K shortest path using the Yen algorithm
     *
     * @param G The OrientedGraph on which we compute the paths
     * @param origin The origin
     * @param destination The destination
     * @param costMetric The cost metric to consider
     * @param accessibleLinkLabels The set of accessible link labels
     * @param labelToCostFamily The cost family to use for each link label
     * @param kPath The number of path to compute
     * @return Vector of length <= kPath (possibly empty), containing the found paths.
     */
    std::vector<PathCost> YenKShortestPath(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const setstring &accessibleLinkLabels,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int kPath)
    {
        LabelToCostFamilyLinkCostFunction costFunction(labelToCostFamily, costMetric);
        std::vector<PathCost> A;
        std::vector<PathCost> B;

        auto initialPath = dijkstraGeneric(G, origin, destination, costFunction, accessibleLinkLabels);

        // Stop here if no path from origin to destination is found.
        if (!initialPath.has_value()) {
            return A;
        }

        A.emplace_back(std::move(*initialPath));

        for (int k = 1; k < kPath; k++)
        {
            const OrientedGraphPath &previousPath = A[k - 1].first;

            // The spur node ranges is made of all the nodes in the previous path, except the last one.
            for (std::size_t i = 0; i < previousPath.size(); i++) {
                const Node *spurNode = previousPath.node(i);

                OrientedGraphPath rootPath = previousPath.subPath(0, i);

                // For each previous shortest path which shares the same root path,
                // flag the link that comes just after spur node as "forbidden",
                // to force the path search function to try another way.
                std::unordered_set<const Link*> forbiddenLinks;
                for (const PathCost &pc : A) {
                    if (pc.first.startsWith(rootPath)) {
                        forbiddenLinks.emplace(pc.first.link(i));
                    }
                }

                // Cost is assigned to +infinity on forbidden links, making them impassable.
                auto overridenCostFunction = [&forbiddenLinks, &costFunction](const Link *link) -> double {
                    auto it = forbiddenLinks.find(link);
                    return it != forbiddenLinks.end() ? INFINITY : costFunction(link);
                };

                auto spurPath = dijkstraGeneric(G, spurNode->mid, destination, overridenCostFunction, accessibleLinkLabels);
                if (!spurPath.has_value()) {
                    continue;
                }

                // Merge the spurPath to the root path to make a new candidate path from origin to destination.
                // After that, `rootPath` becomes a candidate path from "origin" to "destination".
                rootPath.add(spurPath->first);

                bool toAdd = true;
                for (const auto &prevPath : B)
                {
                    if (rootPath == prevPath.first)
                    {
                        toAdd = false;
                        break;
                    }
                }

                if (toAdd)
                {
                    double newPathCost = computePathCostGeneric(rootPath, costFunction);
                    B.emplace_back(std::move(rootPath), newPathCost);
                }
            }

            if (B.empty())
            {
                break;
            }

            std::sort(B.begin(), B.end(), [](const PathCost &a, const PathCost &b) {
                return a.second < b.second;
            });
            A.push_back(B[0]);
            B.erase(B.begin());
        }

        return A;
    }

    /**
     * @brief Batch computation of K shortest paths using openmp, each thread has its own deep copy of the OrientedGraph to ensure that the increase of the cost do not collapse with the other threads
     *
     * @param G The OrientedGraph on which we compute the paths
     * @param origins The origins
     * @param destinations The destinations
     * @param costMetric The cost metric to consider
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param accessibleLinkLabels The vector set of accessible link labels
     * @param maxDiffCost The maximal difference between the cost of the first computed
     *                    shortest path and the cost of the next ones
     * @param maxDistInCommon The maximal distance in common between the first shortest
     *                        path found and the next ones
     * @param costMultiplier The multiplier applied to the links costs of an
     *                       accepted shortest path
     * @param maxRetry Maximum number of times we retry to find an acceptable shorest path
     * @param kPaths The number of paths to compute
     * @param threadNumber Number of threads to use
     */
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
        int threadNumber)
    {
        omp_set_num_threads(threadNumber);

        std::size_t nbODs = origins.size();
        std::vector<std::vector<PathCost>> res(nbODs);

        std::vector<int> uniqueIndices;
        std::unordered_map<int, int> duplicateIndices;
        std::unordered_map<int, int> nbPaths;
        std::vector<std::string> costMetrics(nbODs, costMetric);
        tie(uniqueIndices, duplicateIndices, nbPaths) = find_duplicates(origins, destinations, labelToCostFamily, costMetrics, kPaths);

        // FIXME MSVC is still stuck to OpenMP 2.0, which requires **signed** loop variables for parallel for.
        std::int64_t nbUniqueIndices = uniqueIndices.size();

        #pragma omp parallel for shared(res, accessibleLinkLabels, G, labelToCostFamily, origins, destinations, kPaths)
        for (std::int64_t i = 0; i < nbUniqueIndices; ++i) {
            int uniqueIdx = uniqueIndices[i];
            if (accessibleLinkLabels.empty())
            {
                res[uniqueIdx] = KShortestPath(G, origins[uniqueIdx], destinations[uniqueIdx], costMetric, {}, labelToCostFamily[uniqueIdx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[uniqueIdx], false);
            }
            else
            {
                res[uniqueIdx] = KShortestPath(G, origins[uniqueIdx], destinations[uniqueIdx], costMetric, accessibleLinkLabels[uniqueIdx], labelToCostFamily[uniqueIdx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[uniqueIdx], false);
            }
        }

        // Set shortest paths of duplicates
        for (const auto& elem : duplicateIndices)
        {
            res[elem.first] = res[elem.second];
        }

        // Keep only the proper number of paths for each OD
        for (std::size_t i = 0; i < nbODs; ++i)
        {
            std::size_t k = kPaths[i];
            std::vector<PathCost> res_paths = res[i];
            if (res_paths.size() > k)
            {
                std::sort(res_paths.begin(), res_paths.end(), [](const PathCost &a, const PathCost &b) {
                    return a.second < b.second;
                });
                std::vector<PathCost> res_k_best_paths(res_paths.begin(), res_paths.begin() + k);
                res[i] = res_k_best_paths;
            }
        }

        return res;
    }

    /**
     * @brief Compute the shortest between origin and destination using the A* algorithm
     *
     * @param G The OrientedGraph on which we compute the path
     * @param origin The origin
     * @param destination The destination
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param labelToCostFamily The cost family to use for each link label
     * @param accessibleLinkLabels The set of accessible link labels
     * @param heuristic An heuristic to speed up the shortest path computation
     * @return Shortest path + the associated cost, or std::nullopt if no path exist from origin to destination.
     */
    std::optional<PathCost> aStar(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels,
        const std::function<double(const Node *, const Node *)> &heuristic)
    {
        const Node *origin_node = G.mnodes.at(origin);
        const Node *destination_node = G.mnodes.at(destination);

        // FIXME This is mostly the same implementation as dijkstra(): in fact, dijkstra() is equivalent to A*
        // with a heuristic function that always returns 0. Could be refactored to avoid code duplication.

        // Return an empty path (i.e. no link) with zero cost if origin and destination are identical.
        if (origin_node == destination_node) {
            return PathCost{ OrientedGraphPath(origin_node), 0 };
        }

        // TODO doc
        std::unordered_map<const Node*, double> dist; // No nullptr keys in this map.
        std::unordered_map<const Node*, const Link*> prev; // No nullptr (neither as key nor as value) in this map.
        dist.reserve(G.mnodes.size());
        prev.reserve(G.mnodes.size());
        dist[origin_node] = 0;

        std::priority_queue<DijkstraQueueItem> pq;
        pq.emplace(DijkstraQueueItem{ heuristic(origin_node, destination_node), origin_node });

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

                    double cost_on_link = link->cost(labelToCostFamily.at(link->mlabel), costMetric);
                    double new_dist = dist_u + cost_on_link;
                    const Node *neighbor = link->mdown;

                    auto neighbor_it = dist.try_emplace(neighbor, INFINITY).first;
                    if (neighbor_it->second > new_dist) { // Follow the link only if it STRICTLY improves the distance.
                        neighbor_it->second = new_dist;
                        pq.emplace(DijkstraQueueItem{ new_dist + heuristic(neighbor, destination_node), neighbor });
                        prev[neighbor] = link;
                    }
                }
            });
        }
        return std::nullopt; // No path exists from origin to destination.
    }

    /**
     * @brief A* algorithm with an euclidian distance as heuristic
     *
     * @param G The OrientedGraph on which we compute the path
     * @param origin The origin
     * @param destination The destination
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param labelToCostFamily The cost family to use for each link label
     * @param accessibleLinkLabels The set of accessible link labels
     * @return Shortest path + the associated cost, or std::nullopt if no path exist from origin to destination.
     */
    std::optional<PathCost> aStarEuclidianDist(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels)
    {
        return aStar(G, origin, destination, costMetric, labelToCostFamily, accessibleLinkLabels,
            [](const Node *current, const Node *dest) {
                double dx = dest->mposition[0] - current->mposition[0];
                double dy = dest->mposition[1] - current->mposition[1];
                return std::sqrt(dx * dx + dy * dy);
            }
        );
    }

    /**
     * @brief Batch computation of intermodal shortest paths with openmp using the
     *        a doubled graph to be sure that the path found is intermodal and Dijkstra algorithm
     *
     * @param G The OrientedGrah used for the shortest paths
     * @param origins The vector of origins
     * @param destinations The vector of destinations
     * @param labelToCostFamily For each OD, the cost family to use for each link label
     * @param costMetric The cost metric to consider in the shortest path algorithm
     * @param threadNumber The number of thread for openmp
     * @param pairMandatoryLabels The pair of labels groups the shortest paths must contain
     * @param kPaths The number of paths to compute
     * @param maxDiffCost The maximal difference between the cost of the first computed
     *                    shortest path and the cost of the next ones
     * @param maxDistInCommon The maximal distance in common between the first shortest
     *                        path found and the next ones
     * @param costMultiplier The multiplier applied to the links costs of an
     *                       accepted shortest path
     * @param maxRetry Maximum number of times we retry to find an acceptable shorest path
     * @param accessibleLinkLabels The vector of accessible link labels
     * @return std::vector<std::vector<pathCost>> The vector of computed shortest path
     */
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
        const std::vector<setstring> &accessibleLinkLabels)
    {
        // Create doubled graph two ways
        OrientedGraph doubledG1; // pass first on first elem of pairMandatoryLabels
        OrientedGraph doubledG2; // pass first on second elem of pairMandatoryLabels
        for(const auto &keyVal: G.mnodes) {
            doubledG1.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::ORIGINAL),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
            doubledG1.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::TWIN),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
            doubledG1.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::TRIPLE),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
            doubledG2.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::ORIGINAL),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
            doubledG2.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::TWIN),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
            doubledG2.AddNode(StrCat(keyVal.second->mid, IntermodalNodeSuffix::TRIPLE),
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
        }
        for(const auto &keyVal: G.mlinks) {
            doubledG1.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::ORGINAL_ORIGINAL),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::ORIGINAL),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::ORIGINAL),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            doubledG1.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TWIN_TWIN),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TWIN),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TWIN),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            doubledG1.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TRIPLE_TRIPLE),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TRIPLE),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TRIPLE),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            doubledG2.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::ORGINAL_ORIGINAL),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::ORIGINAL),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::ORIGINAL),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            doubledG2.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TWIN_TWIN),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TWIN),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TWIN),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            doubledG2.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TRIPLE_TRIPLE),
                            StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TRIPLE),
                            StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TRIPLE),
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
            // Add link on G1 between original and twin graph only if it corresponds to a mandatory label of the first group
            bool original_to_twin_G1 = pairMandatoryLabels.first.count(keyVal.second->mlabel);
            if (original_to_twin_G1) {
                doubledG1.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::ORIGINAL_TWIN),
                                StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::ORIGINAL),
                                StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TWIN),
                                keyVal.second->mlength,
                                keyVal.second->mcosts,
                                keyVal.second->mlabel);
            }
            // Add link on G2 between original and twin graph only if it corresponds to a mandatory label of the second group
            bool original_to_twin_G2 = pairMandatoryLabels.second.count(keyVal.second->mlabel);
            if (original_to_twin_G2) {
                doubledG2.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::ORIGINAL_TWIN),
                                StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::ORIGINAL),
                                StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TWIN),
                                keyVal.second->mlength,
                                keyVal.second->mcosts,
                                keyVal.second->mlabel);
            }
            // Add link on G1 between twin and triple graph only if it corresponds to a mandatory label of the second group
            bool twin_to_trpl_G1 = pairMandatoryLabels.second.count(keyVal.second->mlabel);
            if (twin_to_trpl_G1) {
                doubledG1.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TWIN_TRIPLE),
                                StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TWIN),
                                StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TRIPLE),
                                keyVal.second->mlength,
                                keyVal.second->mcosts,
                                keyVal.second->mlabel);
            }
            // Add link on G2 between twin and triple graph only if it corresponds to a mandatory label of the first group
            bool twin_to_trpl_G2 = pairMandatoryLabels.first.count(keyVal.second->mlabel);
            if (twin_to_trpl_G2) {
                doubledG2.AddLink(StrCat(keyVal.second->mid, IntermodalLinkSuffix::TWIN_TRIPLE),
                                StrCat(keyVal.second->mup->mid, IntermodalNodeSuffix::TWIN),
                                StrCat(keyVal.second->mdown->mid, IntermodalNodeSuffix::TRIPLE),
                                keyVal.second->mlength,
                                keyVal.second->mcosts,
                                keyVal.second->mlabel);
            }
        }

        // Set origins (resp. destinations) as nodes of the original (resp. triple) graph
        std::vector<std::string> originsTwin;
        std::vector<std::string> destinationsTwin;
        originsTwin.reserve(origins.size());
        destinationsTwin.reserve(destinations.size());
        for (const auto &origin : origins) {
            originsTwin.emplace_back(StrCat(origin, IntermodalNodeSuffix::ORIGINAL));
        }
        for (const auto &destination : destinations) {
            destinationsTwin.emplace_back(StrCat(destination, IntermodalNodeSuffix::TRIPLE));
        }

        // Launch dijkstra algo for each OD in parallel
        omp_set_num_threads(threadNumber);

        std::size_t nbOD = origins.size();
        std::vector<std::vector<PathCost>> res(nbOD);

        std::vector<int> uniqueIndices;
        std::unordered_map<int, int> duplicateIndices;
        std::unordered_map<int, int> nbPaths;
        std::vector<std::string> costMetrics(nbOD, costMetric);
        tie(uniqueIndices, duplicateIndices, nbPaths) = find_duplicates(origins, destinations, labelToCostFamily, costMetrics, kPaths);

        // FIXME MSVC is still stuck to OpenMP 2.0, which requires **signed** loop variables for parallel for.
        std::int64_t nbUniqueIndices = uniqueIndices.size();

        #pragma omp parallel for shared(res, accessibleLinkLabels, labelToCostFamily, originsTwin, destinationsTwin, kPaths, doubledG1, doubledG2)
        for (std::int64_t i = 0; i < nbUniqueIndices; i++)
        {
            int idx = uniqueIndices[i];
            std::vector<PathCost> resPath1;
            std::vector<PathCost> resPath2;

            if (accessibleLinkLabels.empty())
            {
                // Look for shortest paths on G1
                resPath1 = KShortestPath(doubledG1, originsTwin[idx], destinationsTwin[idx], costMetric, {}, labelToCostFamily[idx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[idx], true);
                // Look for shortest paths on G2
                resPath2 = KShortestPath(doubledG2, originsTwin[idx], destinationsTwin[idx], costMetric, {}, labelToCostFamily[idx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[idx], true);
            }
            else
            {
                resPath1 = KShortestPath(doubledG1, originsTwin[idx], destinationsTwin[idx], costMetric, accessibleLinkLabels[idx], labelToCostFamily[idx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[idx], true);
                resPath2 = KShortestPath(doubledG2, originsTwin[idx], destinationsTwin[idx], costMetric, accessibleLinkLabels[idx], labelToCostFamily[idx], maxDiffCost, maxDistInCommon, costMultiplier, maxRetry, nbPaths[idx], true);
            }
            /* TODO replug // Concat resPath1 and resPath2
            resPath1.insert(resPath1.end(), resPath2.begin(), resPath2.end());
            // Decode paths
            for (auto &path1 : resPath1) {
                path1.first = decodeIntermodalPath(path1.first);
            }
            // Keep only unique paths
            sort( resPath1.begin(), resPath1.end() );
            resPath1.erase(std::unique( resPath1.begin(), resPath1.end() ), resPath1.end() );

            // Keep the k best paths found
            std::sort(resPath1.begin(), resPath1.end(), [](const pathCost &a, const pathCost &b) {
                return a.second < b.second;
            });
            std::size_t currentNbPaths = nbPaths[idx];
            if (resPath1.size() >= currentNbPaths)
            {
                std::vector<pathCost> resPath(resPath1.begin(), resPath1.begin() + currentNbPaths);
                res[idx] = resPath;
            }
            else
            {
                res[idx] = resPath1;
            }*/
        }

      // Set shortest paths of duplicates
      for (const auto& elem : duplicateIndices)
      {
          res[elem.first] = res[elem.second];
      }

      // Keep only the proper number of paths for each OD
      for (std::size_t i = 0; i < nbOD; ++i)
      {
          std::size_t k = kPaths[i];
          std::vector<PathCost> res_paths = res[i];
          if (res_paths.size() > k)
          {
              std::sort(res_paths.begin(), res_paths.end(), [](const PathCost &a, const PathCost &b) {
                return a.second < b.second;
              });
              std::vector<PathCost> res_k_best_paths(res_paths.begin(), res_paths.begin() + k);
              res[i] = res_k_best_paths;
          }
      }

      return res;
    }

} // namespace hipop
