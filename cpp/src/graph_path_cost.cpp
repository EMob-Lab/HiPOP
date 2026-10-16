#include <hipop/graph_path_cost.h>

#include <omp.h>

#include <cstdint>


namespace hipop {


    std::vector<std::vector<double>> computePathsCosts(
        const std::vector<std::vector<OrientedGraphPath>> &paths,
        const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily,
        int threadNumber
    ) {
        omp_set_num_threads(threadNumber);

        // FIXME MSVC is still stuck to OpenMP 2.0, which requires **signed** loop variables for parallel for.
        std::int64_t nbBatches = paths.size();

        LabelToCostFamilyLinkCostFunction costFunction(labelToCostFamily, costMetric);
        std::vector<std::vector<double>> result(nbBatches);

        #pragma omp parallel for
        for (std::int64_t i = 0; i < nbBatches; i++) {

            const std::vector<OrientedGraphPath> &currentPaths = paths[i];
            std::vector<double> &currentResult = result[i];
            currentResult.reserve(currentPaths.size());

            for (const OrientedGraphPath &path : currentPaths) {
                currentResult.emplace_back(computePathCostGeneric(path, costFunction));
            }
        }

        return result;
    }


}
