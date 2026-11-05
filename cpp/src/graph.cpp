#include <hipop/graph.h>

#include <hipop/string_util.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hipop {

    void OrientedGraph::AddAllNodesAndLinks(const OrientedGraph &graph) {
        for (const Node *otherNode : graph.nodes()) {
            AddNode(
                otherNode->mid,
                otherNode->mposition[0],
                otherNode->mposition[1],
                otherNode->mlabel,
                otherNode->mexclude_movements
            );
        }
        for (const Link *otherLink : graph.links()) {
            AddLink(
                otherLink->mid,
                otherLink->mup->mid,
                otherLink->mdown->mid,
                otherLink->mlength,
                otherLink->mcosts,
                otherLink->mlabel
            );
        }
    }


    // NOLINTBEGIN(performance-unnecessary-value-param): false positive, clang-tidy loses track of the
    // std::move(..) once they are forwarded through try_emplace(..) down to the object constructor.
    void OrientedGraph::AddNode(std::string nodeId, double x, double y, std::string label,
        mapsets excludeMovements) {
    // NOLINTEND(performance-unnecessary-value-param)

        std::string key = nodeId; // Copy mandatory because of std::move(..).

        auto [it, inserted] = nodes_.try_emplace(std::move(key), std::move(nodeId), x, y, std::move(label),
            std::move(excludeMovements));
        if (!inserted) {

            // WARNING: do not use the nodeId received in input for the error message, as it may have been moved.
            throw std::invalid_argument(StrCat("[OrientedGraph::AddNode] Duplicated node ID [", it->first, "]."));
        }
    }


    // NOLINTBEGIN(performance-unnecessary-value-param): false positive, clang-tidy loses track of the
    // std::move(..) once they are forwarded through try_emplace(..) down to the object constructor.
    void OrientedGraph::AddLink(std::string linkId, const std::string &up, const std::string &down,
        double length, mapcosts costs, std::string label) {
    // NOLINTEND(performance-unnecessary-value-param)

        Node *up_node = node(up);
        Node *down_node = node(down);
        std::string key = linkId; // Copy mandatory because of std::move(..).

        auto [it, inserted] = links_.try_emplace(std::move(key), std::move(linkId), up_node, down_node, length,
            std::move(costs), std::move(label));
        if (!inserted) {

            // WARNING: do not use the linkId received in input for the error message, as it may have been moved.
            throw std::invalid_argument(StrCat("[OrientedGraph::AddLink] Duplicated link ID [", it->first, "]."));
        }

        Link *new_link = &it->second;
        up_node->madj.emplace(down, new_link);
        down_node->mradj.emplace(up, new_link);
    }


    void OrientedGraph::DeleteNode(const std::string &nodeId) {

        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return;
        }

        Node *node = &it->second;

        // Remove the entrance links.
        // WARNING: do not remove links while iterating on the adjacency map
        // (because link deletion would invalidate the iterators on this map).
        std::vector<Link *> linksToRemove;
        linksToRemove.reserve(std::max(node->madj.size(), node->mradj.size()));
        for (const auto &linkIt : node->madj) {
            linksToRemove.emplace_back(linkIt.second);
        }
        for (Link *link : linksToRemove) {
            DeleteLink(link->mid);
        }

        // Same thing for the exit links.
        linksToRemove.clear();
        for (const auto &linkIt : node->mradj) {
            linksToRemove.emplace_back(linkIt.second);
        }
        for (Link *link : linksToRemove) {
            DeleteLink(link->mid);
        }

        // Remove the node itself at the end.
        nodes_.erase(it);
    }


    void OrientedGraph::DeleteLink(const std::string &linkId) {

        auto it = links_.find(linkId);
        if (it == links_.end()) {
            return;
        }

        Link *link = &it->second;

        // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
        // Casting away const is safe here: the nodes are owned by this graph, which is itself
        // non-const in this method, so the nodes are not actually const objects.
        Node *up = const_cast<Node *>(link->mup);
        Node *down = const_cast<Node *>(link->mdown);
        // NOLINTEND(cppcoreguidelines-pro-type-const-cast)

        // Remove the references to the link in its adjacent nodes, and the link itself at the end.
        up->madj.erase(down->mid);
        down->mradj.erase(up->mid);
        links_.erase(it);
    }


    /**
     * @brief Update a Link costs
     *
     * @param linkId The id of the Link to update
     * @param costs The new costs
     */
    void OrientedGraph::UpdateLinkCosts(const std::string &linkId, mapcosts costs) {
        link(linkId)->updateCosts(std::move(costs));
    }

    /**
     * @brief Update a list of link costs
     *
     * @param linkIdToCosts The map of the links/costs to update
     */
    void OrientedGraph::UpdateCosts(const std::unordered_map<std::string, mapcosts> &linkIdToCosts)
    {
        for (const auto &it : linkIdToCosts) {
            UpdateLinkCosts(it.first, it.second);
        }
    }


    void OrientedGraph::ShowNodes() const {
        for (const Node *node : nodes()) {
            std::cout << "Node(" << node->mid << ", [" << node->mposition[0] << ",\t" << node->mposition[1] << "])\n";
        }
    }


    void OrientedGraph::ShowLinks() const {
        for (const Link *link : links()) {
            std::cout << "Link(" << link->mid << ", " << link->mup->mid << ", " << link->mdown->mid << ")\n";
        }
    }


    /**
     * @brief Get the length of a link
     *
     * @param up the upstream node of the link
     * @param down the downstream node of the link
     * @return double the length of the link
     */
    double OrientedGraph::getLength(const std::string &up, const std::string &down)
    {
        return node(up)->madj[down]->mlength;
    }


    /**
     * @brief Merge multiple OrientedGraph together into one
     *
     * @param graphs Vector of OrientedGraph to merge
     * @return OrientedGraph* The result of the merge
     */
    OrientedGraph mergeOrientedGraph(const std::vector<const OrientedGraph*> &graphs) {
        OrientedGraph newGraph;
        for (const OrientedGraph *G : graphs) {
            newGraph.AddAllNodesAndLinks(*G);
        }
        return newGraph;
    }


    std::vector<std::string> OrientedGraph::GetLinksWithoutCost(const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily) const {

        std::vector<std::string> result;
        for (const Link *link : links()) {
            if (!link->hasCost(labelToCostFamily.at(link->mlabel), costMetric)) {
                result.emplace_back(link->mid);
            }
        }
        return result;
    }


} // namespace hipop
