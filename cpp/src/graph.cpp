#include <hipop/graph.h>

#include <hipop/string_util.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hipop
{
    OrientedGraph::OrientedGraph(const OrientedGraph &other) {
        AddAllNodesAndLinks(other);
    }


    OrientedGraph &OrientedGraph::operator=(const OrientedGraph &other) {
        if (this != &other) {

            // Create a temporary OrientedGraph instance + move-assign it to `this`.
            // This ensures that the current OrientedGraph instance remains unchanged on failure.
            *this = OrientedGraph(other);
        }
        return *this;
    }


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


    /**
     * @brief Delete a link from the OrientedGraph
     */
    void OrientedGraph::DeleteLink(const std::string &linkId) {

        if (links_.find(linkId) != links_.end())
        {
            Link* pLink = &links_.at(linkId);

            if (nodes_.find(pLink->mup->mid) != nodes_.end())
            {
                Node* pUp = &nodes_.at(pLink->mup->mid);
                pUp->madj.erase(pLink->mdown->mid);
            }

            if (nodes_.find(pLink->mdown->mid) != nodes_.end())
            {
                Node* pDown = &nodes_.at(pLink->mdown->mid);
                pDown->mradj.erase(pLink->mup->mid);
            }

            links_.erase(linkId);
        }
    };

    /**
     * @brief Delete all links to a specific node
     */
    void OrientedGraph::DeleteAllLinksToNode(const std::string &nodeId) {

        if (nodes_.find(nodeId) != nodes_.end())
        {
            for (const auto &it : nodes_.at(nodeId).madj) {
                DeleteLink(it.second->mid);
            }
        }

    };

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

    /**
     * @brief Print the Nodes informations
     *
     */
    void OrientedGraph::ShowNodes() {
        for(const auto &elem: nodes_) {
            std::cout << "Node(" << elem.first << ", [" << elem.second.mposition[0] << ",\t" << elem.second.mposition[1] << "])\n";
        }
    }

    /**
     * @brief Print the Link informations
     *
     */
    void OrientedGraph::ShowLinks() {
        for(const auto &elem: links_) {
            std::cout << "Link(" << elem.first << ", " << elem.second.mup->mid << ", " << elem.second.mdown->mid << ")\n";
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

    /**
     * @brief Return links for which the considered cost has not been set
     *
     * @param costMetric The cost metric to consider
     * @param labelToCostFamily The cost family to use for each link label
     * @return std::vector<std::string> The list of links with no cost
     */
    std::vector<std::string> OrientedGraph::GetLinksWithoutCost(const std::string &costMetric, const std::unordered_map<std::string, std::string> &labelToCostFamily)
    {
        std::vector<std::string> links;
        for(auto &elem: links_)
        {
            if ( elem.second.mcosts[labelToCostFamily.at(elem.second.mlabel)].find(costMetric) == elem.second.mcosts[labelToCostFamily.at(elem.second.mlabel)].end() )
                links.push_back(elem.first);
        }

        return links;
    }
} // namespace hipop
