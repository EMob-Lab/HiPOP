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

            // FIXME: duplicated code with destructor. Could be improved with smart pointers...
            for (const auto &it : links_) {
                delete it.second;
            }
            for (const auto &it : nodes_) {
                delete it.second;
            }

            links_.clear();
            nodes_.clear();
            AddAllNodesAndLinks(other);
        }
        return *this;
    }


    OrientedGraph::~OrientedGraph() {
        for (const auto &it : links_) {
            delete it.second;
        }
        for (const auto &it : nodes_) {
            delete it.second;
        }
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


    void OrientedGraph::AddNode(std::string nodeId, double x, double y, std::string label,
        mapsets excludeMovements) {

        if (nodes_.find(nodeId) != nodes_.end()) {
            throw std::invalid_argument(StrCat("[OrientedGraph::AddNode] Duplicated node ID [", nodeId, "]."));
        }

        Node *new_node = new Node(std::move(nodeId), x, y, std::move(label), std::move(excludeMovements));
        nodes_[new_node->mid] = new_node;
    }


    void OrientedGraph::AddLink(std::string linkId, const std::string &up, const std::string &down,
        double length, mapcosts costs, std::string label) {

        Node *up_node = node(up);
        Node *down_node = node(down);

        if (links_.find(linkId) != links_.end()) {
            throw std::invalid_argument(StrCat("[OrientedGraph::AddLink] Duplicated link ID [", linkId, "]."));
        }

        Link *new_link = new Link(std::move(linkId), up_node, down_node, length, std::move(costs), std::move(label));
        up_node->madj.emplace(down, new_link);
        down_node->mradj.emplace(up, new_link);
        links_.emplace(new_link->mid, new_link);
    }


    /**
     * @brief Delete a link from the OrientedGraph
     */
    void OrientedGraph::DeleteLink(const std::string &linkId) {

        if (links_.find(linkId) != links_.end())
        {
            Link* pLink = links_[linkId];

            if (nodes_.find(pLink->mup->mid) != nodes_.end())
            {
                Node* pUp = nodes_[pLink->mup->mid];
                pUp->madj.erase(pLink->mdown->mid);
            }

            if (nodes_.find(pLink->mdown->mid) != nodes_.end())
            {
                Node* pDown = nodes_[pLink->mdown->mid];
                pDown->mradj.erase(pLink->mup->mid);
            }

            links_.erase(linkId);
            delete pLink;
        }
    };

    /**
     * @brief Delete all links to a specific node
     */
    void OrientedGraph::DeleteAllLinksToNode(const std::string &nodeId) {

        if (nodes_.find(nodeId) != nodes_.end())
        {
            for (const auto &it : nodes_[nodeId]->madj) {
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
        links_[linkId]->updateCosts(std::move(costs));
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
            std::cout << "Node(" << elem.first << ", [" << elem.second->mposition[0] << ",\t" << elem.second->mposition[1] << "])\n";
        }
    }

    /**
     * @brief Print the Link informations
     *
     */
    void OrientedGraph::ShowLinks() {
        for(const auto &elem: links_) {
            std::cout << "Link(" << elem.first << ", " << elem.second->mup->mid << ", " << elem.second->mdown->mid << ")\n";
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
        return nodes_[up]->madj[down]->mlength;
    }


    /**
     * @brief Merge multiple OrientedGraph together into one
     *
     * @param graphs Vector of OrientedGraph to merge
     * @return OrientedGraph* The result of the merge
     */
    OrientedGraph* mergeOrientedGraph(const std::vector<const OrientedGraph*> &graphs) {
        auto newGraph = new OrientedGraph();
        for (const OrientedGraph *G : graphs) {
            newGraph->AddAllNodesAndLinks(*G);
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
        for(const auto &elem: links_)
        {
            if ( elem.second->mcosts[labelToCostFamily.at(elem.second->mlabel)].find(costMetric) == elem.second->mcosts[labelToCostFamily.at(elem.second->mlabel)].end() )
                links.push_back(elem.first);
        }

        return links;
    }
} // namespace hipop
