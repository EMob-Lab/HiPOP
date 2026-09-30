#pragma once

#include <hipop/shortest_path.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>


inline void assertTrue(bool test, std::string_view message) {
    if (!test) {
        std::cerr << "[ERROR] " << message << '\n';
        throw std::runtime_error(static_cast<std::string>(message));
    }
}


/**
 * Whether a and b are close to each other, up to a given maximum absolute difference.
 *
 * @param epsilon Must be >= 0, and "small" with respect to the order of magnitude of a and b.
 */
inline bool isClose(double a, double b, double epsilon) {
    if (std::isfinite(a) && std::isfinite(b)) {
        return std::fabs(a - b) <= epsilon;
    }
    else {
        return (std::isnan(a) && std::isnan(b)) || a == b;
    }
}


inline void assertEqualPaths(const pathCost &actual, const pathCost &expected, std::string_view message) {

    /**
     * We assume that 10^-6 is an acceptable tolerance for comparing the path costs,
     * at least in the context of the unit tests.
     */
    static constexpr double EPSILON = 1e-6;

    bool node_lists_are_equal = expected.first == actual.first;
    bool costs_are_close = isClose(expected.second, actual.second, EPSILON);

    if (!node_lists_are_equal || !costs_are_close) {
        std::cerr << "[ERROR] " << message << '\n';

        auto print = [](const pathCost &path) {
            std::cerr << "[";
            bool is_first_node = true;
            for (const std::string &node : path.first) {
                if (!is_first_node) {
                    std::cerr << " ";
                }
                std::cerr << node;
                is_first_node = false;
            }
            std::cerr << "], size=" << path.first.size() << ", cost=" << path.second << '\n';
        };

        std::cerr << "  actual:   ";
        print(actual);
        std::cerr << "  expected: ";
        print(expected);

        throw std::runtime_error(static_cast<std::string>(message));
    }
}


inline void assertEqualShortestPathsTrees(const ShortestPathsTree &actual, const ShortestPathsTree &expected,
    std::string_view message) {

    if (actual != expected) {
        std::cerr << "[ERROR] " << message << '\n';

        auto print = [](const ShortestPathsTree &tree) {
            std::vector<std::pair<std::string, std::string>> sorted(tree.begin(), tree.end());
            std::sort(sorted.begin(), sorted.end());
            for (const auto &it : sorted) {
                std::cerr << "    " << it.first << " -> " << (it.second.empty() ? "<no predecessor>" : it.second) << '\n';
            }
        };

        std::cerr << "  actual:\n";
        print(actual);
        std::cerr << "  expected:\n";
        print(expected);

        throw std::runtime_error(static_cast<std::string>(message));
    }
}
