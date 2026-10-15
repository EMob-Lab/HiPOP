#pragma once

#include <hipop/graph.h>
#include <hipop/graph_path.h>
#include <hipop/string_util.h>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


namespace hipop {


    OrientedGraph* makeManhattan(int n, double linkLength);


    /**
     * Generate an empty path (i.e. a path with no link) relating the node corresponding to originNodeId
     * to itself in the given graph.
     *
     * @throws std::invalid_argument if the given node ID does not exist in the graph.
     */
    inline OrientedGraphPath makeEmptyPath(const OrientedGraph &graph, const std::string &originNodeId) {
        auto it = graph.mnodes.find(originNodeId);
        if (it == graph.mnodes.end()) {
            throw std::invalid_argument(StrCat("[makeEmptyPath] Unknown node ID [", originNodeId, "]."));
        }
        return OrientedGraphPath(it->second);
    }


    /**
     * Generate a path made of the links corresponding to the given link IDs in the given graph.
     *
     * @param linkIds Must not be empty.
     * @throws std::invalid_argument if linkIds is empty, if any link ID does not exist in the graph,
     *                               or if the sequence of links has inconsistent up/down nodes.
     */
    inline OrientedGraphPath makePath(const OrientedGraph &graph, const std::vector<std::string> &linkIds) {

        if (linkIds.empty()) {
            throw std::invalid_argument("[makePath] Link list cannot be empty.");
        }

        std::vector<const Link *> links;
        links.reserve(linkIds.size());
        for (const std::string &linkId : linkIds) {
            auto it = graph.mlinks.find(linkId);
            if (it == graph.mlinks.end()) {
                throw std::invalid_argument(StrCat("[makePath] Unknown link ID [", linkId, "]."));
            }
            links.emplace_back(it->second);
        }

        const Node *origin = links[0]->mup;
        return { origin, std::move(links) };
    }


} // namespace hipop
