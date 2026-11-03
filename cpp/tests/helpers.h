#pragma once

#include <hipop/shortest_path.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


inline void assertTrue(bool test, std::string_view message) {
    if (!test) {
        std::cerr << "[ERROR] " << message << '\n';
        throw std::runtime_error(static_cast<std::string>(message));
    }
}


/**
 * Check that invoking `callable` throws an exception of type `Exception` (or of a derived type).
 */
template<typename Exception>
void assertThrows(const std::function<void()> &callable, std::string_view message) {
    try {
        callable();
    }
    catch (const Exception &) {
        return;
    }
    catch (...) {
        std::cerr << "[ERROR] " << message << " (unexpected exception type)\n";
        throw std::runtime_error(static_cast<std::string>(message));
    }
    std::cerr << "[ERROR] " << message << " (no exception thrown)\n";
    throw std::runtime_error(static_cast<std::string>(message));
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


inline void assertEqualPaths(
    const std::optional<hipop::PathCost> &actual,
    const std::optional<hipop::PathCost> &expected,
    std::string_view message
) {
    /**
     * We assume that 10^-6 is an acceptable tolerance for comparing the path costs,
     * at least in the context of the unit tests.
     */
    static constexpr double EPSILON = 1e-6;

    auto print = [](const std::optional<hipop::PathCost> &obj) {
        if (obj) {
            std::cerr << obj->first << " cost=" << obj->second << '\n';
        }
        else {
            std::cerr << "<missing>";
        }
    };

    bool are_identical;
    if (actual && expected) { // Both actual and expected have a value -> compare their content.
        bool paths_are_equal = expected->first == actual->first;
        bool costs_are_close = isClose(expected->second, actual->second, EPSILON);
        are_identical = paths_are_equal && costs_are_close;
    }
    else if (actual || expected) { // Either actual or expected is std::nullopt (but not both) -> error.
        are_identical = false;
    }
    else { // Else both actual and expected are std::nullopt -> OK.
        are_identical = true;
    }

    if (!are_identical) {
        std::cerr << "[ERROR] " << message << '\n';
        std::cerr << "  actual:   ";
        print(actual);
        std::cerr << "  expected: ";
        print(expected);
        throw std::runtime_error(static_cast<std::string>(message));
    }
}


/**
 * @param expected Expected paths, defined as a map destination -> path.
 */
inline void assertEqualSameOriginPathTrees(
    const hipop::SameOriginPathTree &actual,
    const std::unordered_map<const hipop::Node *, hipop::OrientedGraphPath> &expected,
    std::string_view message
) {
    std::unordered_set<const hipop::Node *> unmatchedExpectedDestinations;
    for (const auto &it : expected) {
        unmatchedExpectedDestinations.emplace(it.first);
    }

    // Each destination of `actual` must be expected, with the same path.
    for (const hipop::Node *destination : actual.destinations()) {
        auto it = expected.find(destination);
        if (it == expected.end()) {
            std::cerr << "[ERROR] " << message << " (found a path to " << destination->mid << ", while none was expected)\n";
            throw std::runtime_error(static_cast<std::string>(message));
        }
        if (actual.pathTo(destination) != it->second) {
            std::cerr << "[ERROR] " << message << " (wrong path to " << destination->mid << "):\n";
            std::cerr << "  actual:   " << actual.pathTo(destination) << '\n';
            std::cerr << "  expected: " << it->second << '\n';
            throw std::runtime_error(static_cast<std::string>(message));
        }
        unmatchedExpectedDestinations.erase(destination);
    }

    // Each expected destination must be reachable in `actual`.
    if (!unmatchedExpectedDestinations.empty()) {
        auto it = unmatchedExpectedDestinations.begin();
        std::cerr << "[ERROR] " << message << " (no path to "<< (*it)->mid << " found, while one was expected)\n";
        throw std::runtime_error(static_cast<std::string>(message));
    }
}
