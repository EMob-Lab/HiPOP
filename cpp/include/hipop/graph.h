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

typedef std::set<std::string> setstring;
typedef std::vector<std::string> vecstring;
typedef std::unordered_map<std::string, std::set<std::string> > mapsets;
typedef std::unordered_map<std::string, std::unordered_map<std::string, double> > mapcosts;


namespace hipop
{
    class Node;

    class Link {
    public:
        std::string mid;
        const Node *mup;
        const Node *mdown;
        mapcosts mcosts;
        std::string mlabel;
        double mlength;

        Link(std::string id, const Node *up, const Node *down, double length, mapcosts costs, std::string label = "") :
            mid(std::move(id)),
            mup(up),
            mdown(down),
            mcosts(std::move(costs)),
            mlabel(std::move(label)),
            mlength(length)
        {}

        /**
         * Read the cost value associated to the current link, assuming the given mapLabelCost and cost type.
         *
         * @return 0 as default cost value if not explicitely set.
         */
        double getCost(const std::unordered_map<std::string, std::string> &mapLabelCost, const std::string &cost) const {
            auto it1 = mcosts.find(mapLabelCost.at(mlabel));
            if (it1 == mcosts.end()) {
                return 0;
            }
            auto it2 = it1->second.find(cost);
            if (it2 == it1->second.end()) {
                return 0;
            }
            return it2->second;
        }

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

        Node(const Node &other) {
            mid = other.mid.c_str();
            mposition[0] = other.mposition[0];
            mposition[1] = other.mposition[1];

            for(const auto &keyVal: other.madj) {
                Link *l = new Link(*keyVal.second);
                madj[keyVal.first] = l;
            }

            for(const auto &keyVal: other.mradj) {
                Link *l = new Link(*keyVal.second);
                mradj[keyVal.first] = l;
            }

            for(const auto &keyVal: other.mexclude_movements) {
                setstring copy;
                for(const auto &s: keyVal.second) {
                    copy.insert(s.c_str());
                }

                mexclude_movements[keyVal.first] = copy;
            }
        }

        /**
         * TODO document forEachExit
         */
        template<typename Callback>
        void forEachExit(const std::string &predecessor, Callback &&callback) const {
            for (const auto &l : madj) {
                auto it = mexclude_movements.find(predecessor);
                if (it == mexclude_movements.end() || it->second.find(l.second->mdown->mid) == it->second.end()) {
                    callback(const_cast<const Link *>(l.second));
                }
            }
        }

        std::vector<Link*> getExits(const std::string &predecessor = "_default") { // TODO remove
            std::vector<Link*> res;
            for(const auto &l: madj) {
                auto it = mexclude_movements.find(predecessor);
                if (it == mexclude_movements.end() || it->second.find(l.second->mdown->mid) == it->second.end()) {
                    res.push_back(l.second);
                }
            }
            return res;
        }

        std::vector<Link*> getEntrances(const std::string &predecessor) {
            std::vector<Link*> res;
            for(const auto &l: mradj) {
                const std::string &neighbor = l.second->mup->mid;
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

        void AddNode(std::string id, double x, double y, std::string label = "", mapsets excludeMovements = {});
        void AddNode(Node *n);
        void AddLink(std::string id, const std::string &up, const std::string &down, double length, mapcosts costs, std::string label = "");
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

        OrientedGraph() {};
        OrientedGraph(const OrientedGraph &other) {
            for(const auto &keyVal: other.mnodes) {
                Node *newNode = new Node(*keyVal.second);
                mnodes[keyVal.first] = newNode;
            }

            for(const auto &keyVal: other.mlinks) {
                Link *newLink = new Link(*keyVal.second);
                mlinks[keyVal.first] = newLink;
            }
        }
        ~OrientedGraph();

    };

    OrientedGraph* copyGraph(const OrientedGraph &G);

    OrientedGraph* mergeOrientedGraph(const std::vector<const OrientedGraph*> &allGraphs);

} // namespace hipop
