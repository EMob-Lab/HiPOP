#include "hipop/graph.h"

#include <vector>
#include <string>
#include <utility>
#include <functional>

#pragma once

using pathCost = std::pair<std::vector<std::string>, double>;
using ShortestPathsTree = std::unordered_map<std::string, std::string>;

namespace hipop
{
    double computePathLength(OrientedGraph &G, const std::vector<std::string> &path);

    double computePathCost(OrientedGraph &G,
        const std::vector<std::string> &path,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily);

    std::vector<std::vector<double>> computePathsCosts(OrientedGraph &G,
        const std::vector<std::vector<std::vector<std::string>>> &paths,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int threadNumber);

    pathCost dijkstra(
        const OrientedGraph &G,
        const std::string &origin,
        const std::string &destination,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        const setstring &accessibleLinkLabels = {});
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
        OrientedGraph &G,
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
        OrientedGraph &G,
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
