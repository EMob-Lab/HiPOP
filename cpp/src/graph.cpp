#include <hipop/graph.h>

#include <hipop/string_util.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace hipop {

    void OrientedGraph::AddAllNodesAndLinks(const OrientedGraph &graph) {
        for (const Node *otherNode : graph.nodes()) {
            AddNode(
                otherNode->id(),
                otherNode->x(),
                otherNode->y(),
                otherNode->label(),
                otherNode->excludeMovements()
            );
        }
        for (const Link *otherLink : graph.links()) {
            AddLink(
                otherLink->id(),
                otherLink->up()->id(),
                otherLink->down()->id(),
                otherLink->length(),
                otherLink->costs(),
                otherLink->label()
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
        new_link->upContainerIdx_ = up_node->outLinks_.size();
        new_link->downContainerIdx_ = down_node->inLinks_.size();
        up_node->outLinks_.emplace_back(new_link);
        down_node->inLinks_.emplace_back(new_link);
        up_node->outLinksPerDownNode_.emplace(down_node, new_link);
    }


    void OrientedGraph::DeleteNode(const std::string &nodeId) {

        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return;
        }

        Node *node = &it->second;

        // Remove the incoming links.
        // WARNING: do not remove links while iterating on the in-link vector
        // (because link deletion would invalidate the iterators on this vector).
        std::vector<const Link *> linksToRemove;
        linksToRemove.reserve(std::max(node->inLinks_.size(), node->outLinks_.size()));
        linksToRemove.insert(linksToRemove.end(), node->inLinks_.begin(), node->inLinks_.end());
        for (const Link *link : linksToRemove) {
            DeleteLink(link->id());
        }

        // Same thing for the outgoing links.
        linksToRemove.clear();
        linksToRemove.insert(linksToRemove.end(), node->outLinks_.begin(), node->outLinks_.end());
        for (const Link *link : linksToRemove) {
            DeleteLink(link->id());
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
        Node *up = link->up_;
        Node *down = link->down_;

        // Remove the reference to the link in the up-to-down link map.
        auto [ upToDownLinksBegin, upToDownLinksEnd ] = up->outLinksPerDownNode_.equal_range(down);
        for (auto entryIt = upToDownLinksBegin; entryIt != upToDownLinksEnd; ++entryIt) {
            if (entryIt->second == link) {
                up->outLinksPerDownNode_.erase(entryIt);
                break;
            }
        }

        // Remove the reference to the link from the outgoing link container of the up node.
        std::size_t upContainerTargetSize = up->outLinks_.size() - 1;
        if (link->upContainerIdx_ < upContainerTargetSize) {
            Link *relocatedLink = up->outLinks_[upContainerTargetSize];
            up->outLinks_[link->upContainerIdx_] = relocatedLink;
            relocatedLink->upContainerIdx_ = link->upContainerIdx_;
        }
        up->outLinks_.resize(upContainerTargetSize);

        // Same thing for the incoming link container of the down node.
        std::size_t downContainerTargetSize = down->inLinks_.size() - 1;
        if (link->downContainerIdx_ < downContainerTargetSize) {
            Link *relocatedLink = down->inLinks_[downContainerTargetSize];
            down->inLinks_[link->downContainerIdx_] = relocatedLink;
            relocatedLink->downContainerIdx_ = link->downContainerIdx_;
        }
        down->inLinks_.resize(downContainerTargetSize);

        // Remove the link itself at the end.
        links_.erase(it);
    }


    void OrientedGraph::UpdateCosts(std::unordered_map<std::string, mapcosts> linkIdToCosts) {
        for (auto &it : linkIdToCosts) {
            link(it.first)->setCosts(std::move(it.second));
        }
    }


    void OrientedGraph::ShowNodes() const {
        for (const Node *node : nodes()) {
            std::cout << "Node(" << node->id() << ", [" << node->x() << ",\t" << node->y() << "])\n";
        }
    }


    void OrientedGraph::ShowLinks() const {
        for (const Link *link : links()) {
            std::cout << "Link(" << link->id() << ", " << link->up()->id() << ", " << link->down()->id() << ")\n";
        }
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


    std::unordered_set<std::string> OrientedGraph::GetLinksWithoutCost(const std::string &costMetric,
        const std::unordered_map<std::string, std::string> &labelToCostFamily) const {

        std::unordered_set<std::string> result;
        for (const Link *link : links()) {
            if (!link->hasCost(labelToCostFamily.at(link->label()), costMetric)) {
                result.emplace(link->id());
            }
        }
        return result;
    }


    void OrientedGraph::checkInternalConsistency() const {

        // 1st pass: collect the addresses of the nodes and links owned by the current graph.
        //
        // After this pass, `ownedNodes` and `ownedLinks` contain the pointers to all Node/Link instances
        // owned by the graph, and nothing else. No null pointers in these sets.
        //
        // In addition, during this pass:
        // A. Ensure that the key with which the node is registered in the `nodes_` map is actually its ID.
        // B. Ensure that the key with which the link is registered in the `links_` map is actually its ID
        //    (i.e. equivalent of step A, for links).
        std::unordered_set<const Node *> ownedNodes;
        std::unordered_set<const Link *> ownedLinks;
        ownedNodes.reserve(nodes_.size());
        ownedLinks.reserve(links_.size());
        for (const auto &nodeEntry : nodes_) {
            const Node *node = &nodeEntry.second;
            ownedNodes.insert(node);

            // Step A
            if (nodeEntry.first != node->id()) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Node [",
                    node->id(),
                    "] registered with key [",
                    nodeEntry.first,
                    "] distinct from its ID in the main node map."
                ));
            }
        }
        for (const auto &linkEntry : links_) {
            const Link *link = &linkEntry.second;
            ownedLinks.insert(link);

            // Step B
            if (linkEntry.first != link->id()) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Link [",
                    link->id(),
                    "] registered with key [",
                    linkEntry.first,
                    "] distinct from its ID in the main link map."
                ));
            }
        }

        // 2nd pass on the links:
        // C. Ensure that the up/down nodes are non-null and do belong to the current graph.
        // D. Ensure that the link is properly registered in the containers referencing the in/out links of each node.
        // E. Ensure that the link is properly registered in the container used by the connecting link queries.
        for (const Link *link : ownedLinks) {

            // Step C
            const Node *up = link->up();
            const Node *down = link->down();
            if (ownedNodes.find(up) == ownedNodes.end()) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Upstream node not owned by current graph for link [",
                    link->id(),
                    "]."
                ));
            }
            if (ownedNodes.find(down) == ownedNodes.end()) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Downstream node not owned by current graph for link [",
                    link->id(),
                    "]."
                ));
            }

            // Step D
            if (link->upContainerIdx_ >= up->outLinks_.size() || up->outLinks_[link->upContainerIdx_] != link) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Link [",
                    link->id(),
                    "] not properly registered in the adjacency container of its upstream node [",
                    up->id(),
                    "]."
                ));
            }
            if (link->downContainerIdx_ >= down->inLinks_.size() || down->inLinks_[link->downContainerIdx_] != link) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Link [",
                    link->id(),
                    "] not properly registered in the adjacency container of its downstream node [",
                    down->id(),
                    "]."
                ));
            }

            // Step E
            auto [connectingLinksBegin, connectingLinksEnd] = up->outLinksPerDownNode_.equal_range(down);
            auto isCurrentLink = [link](const auto &connectingLinkEntry) { return connectingLinkEntry.second == link; };
            if (std::none_of(connectingLinksBegin, connectingLinksEnd, isCurrentLink)) {
                throw std::logic_error(StrCat(
                    "[OrientedGraph::checkInternalConsistency] Link [",
                    link->id(),
                    "] not properly registered as a connecting link from upstream node [",
                    up->id(),
                    "] to downstream node [",
                    down->id(),
                    "]."
                ));
            }
        }

        // 2nd pass on the nodes:
        // F. Ensure that all the link pointers stored in the in/out link containers are non-null, do belong
        //    to the current graph, and have upstream/downstream nodes corresponding to the current node.
        // G. Ensure that each entry in the connecting link container stores non-null down node and link,
        //    that they do belong to the current graph, and that the link has the expected upstream/downstream nodes.
        // H. Ensure that the total number of entries in the in/out link containers and in the connecting link
        //    containers is identical to the number of links in the graph, i.e. that they do not have duplicated
        //    or extra entries.
        std::size_t inLinkEntryCount = 0;
        std::size_t outLinkEntryCount = 0;
        std::size_t connectingLinkEntryCount = 0;
        for (const Node *node : ownedNodes) {
            inLinkEntryCount += node->inLinks_.size();
            outLinkEntryCount += node->outLinks_.size();
            connectingLinkEntryCount += node->outLinksPerDownNode_.size();

            // Step F
            for (const Link *link : node->inLinks_) {
                if (ownedLinks.find(link) == ownedLinks.end()) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Incoming link not owned by current graph for node [",
                        node->id(),
                        "]."
                    ));
                }
                if (link->down() != node) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Link [",
                        link->id(),
                        "] is registered as an incoming link of node [",
                        node->id(),
                        "] although it has a distinct downstream node."
                    ));
                }
            }
            for (const Link *link : node->outLinks_) {
                if (ownedLinks.find(link) == ownedLinks.end()) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Outgoing link not owned by current graph for node [",
                        node->id(),
                        "]."
                    ));
                }
                if (link->up() != node) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Link [",
                        link->id(),
                        "] is registered as an outgoing link of node [",
                        node->id(),
                        "] although it has a distinct upstream node."
                    ));
                }
            }

            // Step G
            for (auto [down, link] : node->outLinksPerDownNode_) {
                if (ownedNodes.find(down) == ownedNodes.end()) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Connecting link entry with a downstream node "
                        "not owned by current graph for upstream node [",
                        node->id(),
                        "]."
                    ));
                }
                if (ownedLinks.find(link) == ownedLinks.end()) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Connecting link not owned by current graph "
                        "from upstream node [",
                        node->id(),
                        "] to downstream node [",
                        down->id(),
                        "]."
                    ));
                }
                if (link->up() != node || link->down() != down) {
                    throw std::logic_error(StrCat(
                        "[OrientedGraph::checkInternalConsistency] Link [",
                        link->id(),
                        "] is registered as a connecting link from upstream node [",
                        node->id(),
                        "] to downstream node [",
                        down->id(),
                        "] although it has distinct upstream/downstream nodes."
                    ));
                }
            }
        }

        // Step H
        if (inLinkEntryCount != ownedLinks.size()) {
            throw std::logic_error(StrCat(
                "[OrientedGraph::checkInternalConsistency] Total number of entries in the incoming link containers (",
                std::to_string(inLinkEntryCount),
                ") differs from the number of links (",
                std::to_string(ownedLinks.size()),
                ")."
            ));
        }
        if (outLinkEntryCount != ownedLinks.size()) {
            throw std::logic_error(StrCat(
                "[OrientedGraph::checkInternalConsistency] Total number of entries in the outgoing link containers (",
                std::to_string(outLinkEntryCount),
                ") differs from the number of links (",
                std::to_string(ownedLinks.size()),
                ")."
            ));
        }
        if (connectingLinkEntryCount != ownedLinks.size()) {
            throw std::logic_error(StrCat(
                "[OrientedGraph::checkInternalConsistency] Total number of entries in the connecting link containers (",
                std::to_string(connectingLinkEntryCount),
                ") differs from the number of links (",
                std::to_string(ownedLinks.size()),
                ")."
            ));
        }
    }


} // namespace hipop
