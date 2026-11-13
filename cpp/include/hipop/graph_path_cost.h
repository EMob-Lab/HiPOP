#pragma once

#include <hipop/graph.h>
#include <hipop/graph_path.h>

#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>


namespace hipop {


    /**
     * Compute the cost of a path, i.e. the sum of the cost value assigned to each link composing the path.
     *
     * How a cost value is assigned to each link is defined by the given cost function.
     *
     * @param costFunction Must be a callable object with the following signature: `double(const Link *link)`.
     *                     Should be a pure function (i.e. no side effects).
     */
    template<typename CostFunction>
    double computePathCostGeneric(const OrientedGraphPath &path, const CostFunction &costFunction) {
        static_assert(
            std::is_invocable_r_v<double, CostFunction, const Link *>,
            "Cost function must have signature: double(const Link *link)"
        );
        double result = 0;
        for (const Link *link : path.links()) {
            result += costFunction(link);
        }
        return result;
    }


    /**
     * Cost function on a link, using the length as cost value.
     */
    struct LengthLinkCostFunction {
        double operator()(const Link *link) const {
            return link->mlength;
        }
    };


    /**
     * Cost function on a link, using fixed a cost family + a fixed cost metric.
     */
    class FixedLinkCostFunction {

        std::string costFamily_;
        std::string costMetric_;

    public:

        FixedLinkCostFunction(std::string costFamily, std::string costMetric) :
            costFamily_(std::move(costFamily)),
            costMetric_(std::move(costMetric)) {}

        double operator()(const Link *link) const {
            return link->cost(costFamily_, costMetric_);
        }
    };


    /**
     * Cost function on a link, using:
     * - cost family resolved from the link label,
     * - fixed cost metric.
     */
    class LabelToCostFamilyLinkCostFunction {

        std::unordered_map<std::string, std::string> labelToCostFamily_;
        std::string costMetric_;

    public:

        LabelToCostFamilyLinkCostFunction(
            std::unordered_map<std::string, std::string> labelToCostFamily,
            std::string costMetric
        ) :
            labelToCostFamily_(std::move(labelToCostFamily)),
            costMetric_(std::move(costMetric)) {}

        double operator()(const Link *link) const {
            return link->cost(labelToCostFamily_.at(link->mlabel), costMetric_);
        }
    };


    /**
     * Total length of a path, i.e. sum of all link length that composed it.
     */
    inline double computePathLength(const OrientedGraphPath &path) {
        return computePathCostGeneric(path, LengthLinkCostFunction());
    }


    /**
     * Total cost of a path, using the given criteria to pick a cost family / cost metric on each link.
     *
     * @param costMetric Cost metric to consider.
     * @param labelToCostFamily Cost family to use for each link label.
     */
    inline double computePathCost(
        const OrientedGraphPath &path,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily
    ) {
        return computePathCostGeneric(path, LabelToCostFamilyLinkCostFunction(labelToCostFamily, costMetric));
    }


    /**
     * Batch computation of path costs.
     */
    std::vector<std::vector<double>> computePathsCosts(
        const std::vector<std::vector<OrientedGraphPath>> &paths,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int threadNumber);


}
