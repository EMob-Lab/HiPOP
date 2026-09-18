#include "hipop/graph.h"


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <queue>
#include <numeric>
#include <limits>


namespace hipop
{
    /**
     * @brief Destroy the Oriented Graph:: Oriented Graph object
     *
     */
    OrientedGraph::~OrientedGraph() {
        for (const auto &iter : mlinks) {
            delete iter.second;
        }

        for (const auto &iter : mnodes) {
            delete iter.second;
        }

        mnodes.clear();
        mlinks.clear();
    }

    /**
     * @brief Create and Add a new Node to the OrientedGraph
     *
     * @param id The id of the Node
     * @param x The x coordinate of the Node
     * @param y The y coordinate of the Node
     * @param label The optional label associated to the Node
     * @param excludeMovements The map of exclude movements with adjacent Nodes
     */
    void OrientedGraph::AddNode(std::string id, double x, double y, std::string label, mapsets excludeMovements) {
        Node *new_node = new Node(std::move(id), x, y, std::move(label), std::move(excludeMovements));
        mnodes[new_node->mid] = new_node;
    };


    /**
     * @brief Add an exisiting Node to the OrientedGraph
     *
     * @param n
     */
    void OrientedGraph::AddNode(Node* n) {
        mnodes[n->mid] = n;
    };


    /**
     * @brief Create and add a new Link to the OrientedGraph
     *
     * @param id The id of the Link
     * @param up The id of the upstream Node of the Link
     * @param down The id of the downstream Node of the Link
     * @param length The length of the Link
     * @param costs The costs of the Link
     * @param label The optional label of the Link
     */
    void OrientedGraph::AddLink(std::string id, std::string up, std::string down, double length, mapcosts costs, std::string label) {
        Link *new_link = new Link(std::move(id), std::move(up), std::move(down), length, std::move(costs), std::move(label));
        mnodes[new_link->mupstream]->madj.emplace(new_link->mdownstream, new_link);
        mnodes[new_link->mdownstream]->mradj.emplace(new_link->mupstream, new_link);
        mlinks.emplace(new_link->mid, new_link);
    };

    /**
     * @brief Add an existing Link to the OrientedGraph
     *
     * @param l The Link to add
     */
    void OrientedGraph::AddLink(Link *l) {
        mnodes[l->mupstream]->madj[l->mdownstream] = l;
        mnodes[l->mdownstream]->mradj[l->mupstream] = l;

        mlinks[l->mid] = l;
    };

    /**
     * @brief Delete a link from the OrientedGraph
     *
     * @param _id The Link id to delete
     */
    void OrientedGraph::DeleteLink(const std::string &id) {

        if (mlinks.find(id) != mlinks.end())
        {
            Link* pLink = mlinks[id];

            if (mnodes.find(pLink->mupstream) != mnodes.end())
            {
                Node* pUp = mnodes[pLink->mupstream];
                pUp->madj.erase(pLink->mdownstream);
            }

            if (mnodes.find(pLink->mdownstream) != mnodes.end())
            {
                Node* pDown = mnodes[pLink->mdownstream];
                pDown->mradj.erase(pLink->mupstream);
            }

            mlinks.erase(id);
            delete pLink;
        }
    };

    /**
     * @brief Delete all links to a specific node
     *
     * @param _id The Node id to consider
     */
    void OrientedGraph::DeleteAllLinksToNode(const std::string &id) {

        if (mnodes.find(id) != mnodes.end())
        {
            for (const auto &it : mnodes[id]->madj) {
                DeleteLink(it.second->mid);
            }
        }

    };

    /**
     * @brief Update a Link costs
     *
     * @param lid The id of the Link to update
     * @param _costs The new costs
     */
    void OrientedGraph::UpdateLinkCosts(const std::string &lid, mapcosts costs) {
        mlinks[lid]->updateCosts(std::move(costs));
    }

    /**
     * @brief Update a list of link costs
     *
     * @param maplinkcosts The map of the links/costs to update
     */
    void OrientedGraph::UpdateCosts(const std::unordered_map<std::string, mapcosts> &maplinkcosts)
    {
        for (const auto &it : maplinkcosts) {
            UpdateLinkCosts(it.first, it.second);
        }
    }

    /**
     * @brief Print the Nodes informations
     *
     */
    void OrientedGraph::ShowNodes() {
        for(const auto &elem: mnodes) {
            std::cout << "Node(" << elem.first << ", [" << elem.second->mposition[0] << ",\t" << elem.second->mposition[1] << "])\n";
        }
    }

    /**
     * @brief Print the Link informations
     *
     */
    void OrientedGraph::ShowLinks() {
        for(const auto &elem: mlinks) {
            std::cout << "Link(" << elem.first << ", " << elem.second->mupstream << ", " << elem.second->mdownstream << ")\n";
        }
    }

 /**
     * @brief Get the length of a link
     *
     * @param _up the upstream node of the link
     * @param _down the downstream node of the link
     * @return double the length of the link
     */
    double OrientedGraph::getLength(const std::string &up, const std::string &down)
    {
        return mnodes[up]->madj[down]->mlength;
    }

    /**
     * @brief Make a deep copy of an OrientedGraph
     *
     * @param G The graph to copy
     * @return OrientedGraph* The copy
     */
    OrientedGraph* copyGraph(const OrientedGraph &G) {
        auto newGraph = new OrientedGraph();

        for(const auto &keyVal: G.mnodes) {
            // Node *n = new Node();

            newGraph->AddNode(keyVal.second->mid,
                            keyVal.second->mposition[0],
                            keyVal.second->mposition[1],
                            keyVal.second->mlabel,
                            keyVal.second->mexclude_movements);
        }


        for(const auto &keyVal: G.mlinks) {
            // Link *l = new Link(*keyVal.second);
            newGraph->AddLink(keyVal.second->mid,
                            keyVal.second->mupstream,
                            keyVal.second->mdownstream,
                            keyVal.second->mlength,
                            keyVal.second->mcosts,
                            keyVal.second->mlabel);
        }

        return newGraph;
    }


    /**
     * @brief Merge multiple OrientedGraph together into one
     *
     * @param allGraphs Vector of OrientedGraph to merge
     * @return OrientedGraph* The result of the merge
     */
    OrientedGraph* mergeOrientedGraph(const std::vector<const OrientedGraph*> &allGraphs) {
        auto newGraph = new OrientedGraph();

        for(auto G:allGraphs) {
            for(const auto &keyValNodes:G->mnodes) {
                mapsets excludeMovements;
                for(const auto &keyVal: keyValNodes.second->mexclude_movements) {
                    setstring copy;
                    for(const auto &s: keyVal.second) {
                        copy.insert(s.c_str());
                    }
                    excludeMovements[keyVal.first] = copy;
                }
                newGraph->AddNode(keyValNodes.first, keyValNodes.second->mposition[0], keyValNodes.second->mposition[1], keyValNodes.second->mlabel, excludeMovements);
            }

            for(const auto &keyVal: G->mlinks) {
                newGraph->AddLink(keyVal.first,
                        keyVal.second->mupstream,
                        keyVal.second->mdownstream,
                        keyVal.second->mlength,
                        keyVal.second->mcosts,
                        keyVal.second->mlabel);

            }
        }

        return newGraph;
    }

    /**
     * @brief Return links for which the considered cost has not been set
     *
     * @param cost The cost to consider
     * @param mapLabelCost The type of cost map to choose on each label
     * @return std::vector<std::string> The list of links with no cost
     */
    std::vector<std::string> OrientedGraph::GetLinksWithoutCost(const std::string &cost, const std::unordered_map<std::string, std::string> &mapLabelCost)
    {
        std::vector<std::string> links;
        for(const auto &elem: mlinks)
        {
            if ( elem.second->mcosts[mapLabelCost.at(elem.second->mlabel)].find(cost) == elem.second->mcosts[mapLabelCost.at(elem.second->mlabel)].end() )
                links.push_back(elem.first);
        }

        return links;
    }
} // namespace hipop
