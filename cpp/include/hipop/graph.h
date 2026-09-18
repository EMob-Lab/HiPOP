#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <set>
#include <vector>
#include <array>
#include <utility>

#include <iostream>

#pragma once

using setstring = std::set<std::string>;
using vecstring = std::vector<std::string>;
using mapsets = std::unordered_map<std::string, std::set<std::string>>;
using mapcosts = std::unordered_map<std::string, std::unordered_map<std::string, double>>;


namespace hipop
{
    class Link {
    public:
        std::string mid;
        std::string mupstream;
        std::string mdownstream;
        mapcosts mcosts;
        std::string mlabel;
        double mlength;

        Link(std::string id, std::string up, std::string down, double length, mapcosts costs, std::string label = "") :
            mid(std::move(id)),
            mupstream(std::move(up)),
            mdownstream(std::move(down)),
            mcosts(std::move(costs)),
            mlabel(std::move(label)),
            mlength(length)
        {}

        // Relationships between Node and Link instances are materialized through raw pointers.
        // Copying and/or moving these instances would break these relationships.
        Link(const Link &other) = delete;
        Link(Link &&other) = delete;
        Link &operator=(const Link &other) = delete;
        Link &operator=(Link &&other) = delete;
        ~Link() = default;

        void updateCosts(mapcosts costs) {
            mcosts = std::move(costs);
        }
    };


    class Node {
    public:
        std::string mid;
        std::array<double, 2> mposition;
        std::unordered_map<std::string, Link* > madj;
        std::unordered_map<std::string, Link* > mradj;
        std::string mlabel;
        mapsets mexclude_movements;

        Node(std::string id, double x, double y, std::string label = "", mapsets exclude_movements = {}) :
            mid(std::move(id)),
            mposition{ x, y },
            mlabel(std::move(label)),
            mexclude_movements(std::move(exclude_movements))
        {}

        // Relationships between Node and Link instances are materialized through raw pointers.
        // Copying and/or moving these instances would break these relationships.
        Node(const Node &other) = delete;
        Node(Node &&other) = delete;
        Node &operator=(const Node &other) = delete;
        Node &operator=(Node &&other) = delete;
        ~Node() = default;

        std::vector<Link*> getExits(const std::string &predecessor = "_default") {
            std::vector<Link*> res;
            for(const auto &l: madj) {
                std::string neighbor = l.second->mdownstream;
                if(mexclude_movements.find(predecessor) == mexclude_movements.end() || mexclude_movements[predecessor].find(neighbor) == mexclude_movements[predecessor].end()) {
                    res.push_back(l.second);
                }
            }
            return res;
        }

        std::vector<Link*> getEntrances(const std::string &predecessor) {
            std::vector<Link*> res;
            for(const auto &l: mradj) {
                std::string neighbor = l.second->mupstream;
                if(mexclude_movements[predecessor].find(neighbor) == mexclude_movements[predecessor].end()) {
                    res.push_back(l.second);
                }
            }
            return res;
        }

    };


    class OrientedGraph {
    public:
        std::unordered_map<std::string, Node* > mnodes;
        std::unordered_map<std::string, Link* > mlinks;

        /**
         * Add all the nodes and links from `other` to the current OrientedGraph.
         *
         * The pre-existing nodes and links of the current OrientedGraph are preserved.
         */
        void AddAllNodesAndLinks(const OrientedGraph &other);

        void AddNode(std::string id, double x, double y, std::string label = "", mapsets excludeMovements = {});
        void AddNode(Node *n);
        void AddLink(std::string id, std::string up, std::string down, double length, mapcosts costs, std::string label = "");
        void AddLink(Link* l);
        void DeleteLink(const std::string &id);
        void DeleteAllLinksToNode(const std::string &id);
        void UpdateLinkCosts(const std::string &lid, mapcosts costs);
        void UpdateCosts(const std::unordered_map<std::string, mapcosts> &maplinkcosts);
        double getLength(const std::string &up, const std::string &down);
        std::vector<std::string> GetLinksWithoutCost(const std::string &cost, const std::unordered_map<std::string, std::string> &mapLabelCost);

        void ShowNodes();
        void ShowLinks();

        Link* getLink(const std::string &id) {
            return mlinks[id];
        }

        OrientedGraph() = default;
        OrientedGraph(const OrientedGraph &other);
        OrientedGraph(OrientedGraph &&other) noexcept = default;
        OrientedGraph &operator=(const OrientedGraph &other);
        OrientedGraph &operator=(OrientedGraph &&other) noexcept = default;
        ~OrientedGraph();

    };

    OrientedGraph* copyGraph(const OrientedGraph &G);

    OrientedGraph* mergeOrientedGraph(const std::vector<const OrientedGraph*> &allGraphs);

} // namespace hipop
