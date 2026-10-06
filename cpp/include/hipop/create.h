#pragma once

#include "hipop/graph.h"
#include "hipop/graph_path.h"

#include <string>
#include <vector>


namespace hipop {


    OrientedGraph* makeManhattan(int n, double linkLength);


    /**
     * Generate an empty path (i.e. a path with no link) relating the node corresponding to originNodeId
     * to itself in the given graph.
     */
    inline OrientedGraphPath makeEmptyPath(const OrientedGraph &graph, const std::string &originNodeId) {
        return OrientedGraphPath(graph.mnodes.at(originNodeId));
    }


    /**
     * Generate a path made of the links corresponding to the given link IDs in the given graph.
     *
     * @param linkIds Must not be empty.
     * @throws std::invalid_argument if linkIds is empty, or if the sequence of links has inconsistent up/down nodes.
     */
    OrientedGraphPath makePath(const OrientedGraph &graph, const std::vector<std::string> &linkIds);


} // namespace hipop
