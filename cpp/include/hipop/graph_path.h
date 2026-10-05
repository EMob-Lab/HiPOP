#pragma once

#include "hipop/graph.h"
#include "hipop/string_util.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>


namespace hipop {


    /**
     * A path within an OrientedGraph - i.e. a sequence of consecutive Link objects,
     * that links an origin Node to a destination Node.
     *
     * Node/Link indexing within a OrientedGraphPath is as follows:
     * ```
     *  Node 0  ---------> Node 1 ---------> Node 2 ... Node N-1 ----------->    Node N
     * (origin)   Link 0            Link 1                         Link N-1   (destination)
     * ```
     * ... where N is the number of links in the path.
     *
     * The data-structure guarantees that, for all link index i:
     * - `link[i]->up() == node[i]`
     * - `link[i]->down() == node[i + 1]`
     * However, the data-structure does not guarantees that "exclude-movement" rules
     * are honored along the path (it's up to the caller in charge of OrientedGraphPath to check that).
     *
     * The size of the path is the number of links, and the number of nodes is size() + 1.
     * In particular, the number of nodes is always > 0, even if the path is empty.
     *
     * Encountering several times the same Node or the same Link in a path is allowed.
     *
     * The OrientedGraph instance from which a path is built must NOT be released
     * before the OrientedGraphPath instance is. Node and Link visited along the path
     * must NOT be removed form the parent graph either. Otherwise, this would result
     * in dangling pointers being held by the OrientedGraphPath instance,
     * and likely segmentation fault on any method invoked on this instance.
     */
    class OrientedGraphPath {

        const Node *origin_;
        std::vector<const Link *> links_;

    public:

        explicit OrientedGraphPath(const Node *origin) : origin_{ origin } {}

        [[nodiscard]] bool operator==(const OrientedGraphPath &other) const {
            return origin_ == other.origin_ && links_ == other.links_;
        }

        [[nodiscard]] bool operator!=(const OrientedGraphPath &other) const {
            return !operator==(other);
        }

        /**
         * Origin - aka. first node - of the path.
         */
        [[nodiscard]] const Node *origin() const {
            return origin_;
        }

        /**
         * Destination - aka. last node - of the path.
         */
        [[nodiscard]] const Node *destination() const {
            return links_.empty() ? origin_ : links_.back()->mdown;
        }

        /**
         * Whether the path is empty (i.e. it has no link) or not.
         */
        [[nodiscard]] bool empty() const {
            return links_.empty();
        }

        /**
         * Number of links in the path.
         *
         * WARNING: the number of nodes in the path is `1 + size()`.
         * In particular, the number of nodes is always > 0, even if the path is empty.
         */
        [[nodiscard]] std::size_t size() const {
            return links_.size();
        }

        /**
         * @param nodeIndex Must be <= size().
         */
        [[nodiscard]] const Node *node(std::size_t nodeIndex) const {
            return nodeIndex == 0 ? origin_ : links_.at(nodeIndex - 1)->mdown;
        }

        /**
         * @param nodeIndex Must be <= size().
         */
        [[nodiscard]] const std::string &nodeId(std::size_t nodeIndex) const {
            return node(nodeIndex)->mid;
        }

        /**
         * @param linkIndex Must be < size().
         */
        [[nodiscard]] const Link *link(std::size_t linkIndex) const {
            return links_.at(linkIndex);
        }

        /**
         * @param linkIndex Must be < size().
         */
        [[nodiscard]] const std::string &linkId(std::size_t linkIndex) const {
            return link(linkIndex)->mid;
        }

        /**
         * Invoke the given callback on each node of the path,
         * in the order in which they are visited along the path.
         *
         * @param callback Must be a callable object with the following signature: `void(const Node *node)`.
         */
        template<typename Callback>
        void forEachNode(Callback &&callback) const {
            static_assert(
                std::is_invocable_v<Callback, const Node *>,
                "[OrientedGraphPath::forEachNode] Wrong callback signature"
            );
            callback(origin_);
            for (const Link *link : links_) {
                callback(link->mdown);
            }
        }

        /**
         * Sequence of nodes visited along the path.
         *
         * @return Guaranteed to be non-empty.
         */
        [[nodiscard]] std::vector<const Node *> nodes() const {
            std::vector<const Node *> result;
            result.reserve(size() + 1);
            forEachNode([&result](const Node *node) { result.emplace_back(node); });
            return result;
        }

        /**
         * Sequence of nodes visited along the path, identified by their IDs.
         *
         * @return Guaranteed to be non-empty.
         */
        [[nodiscard]] std::vector<std::string> nodeIds() const {
            std::vector<std::string> result;
            result.reserve(size() + 1);
            forEachNode([&result](const Node *node) { result.emplace_back(node->mid); });
            return result;
        }

        /**
         * Invoke the given callback on each link of the path,
         * in the order in which they are visited along the path.
         *
         * @param callback Must be a callable object with the following signature: `void(const Link *link)`.
         */
        template<typename Callback>
        void forEachLink(Callback &&callback) const {
            static_assert(
                std::is_invocable_v<Callback, const Link *>,
                "[OrientedGraphPath::forEachLink] Wrong callback signature"
            );
            for (const Link *link : links_) {
                callback(link);
            }
        }

        /**
         * Sequence of links visited along the path.
         */
        [[nodiscard]] const std::vector<const Link *> &links() const {
            return links_;
        }

        /**
         * Sequence of links visited along the path, identified by their IDs.
         */
        [[nodiscard]] std::vector<std::string> linkIds() const {
            std::vector<std::string> result;
            result.reserve(size());
            forEachLink([&result](const Link *link) { result.emplace_back(link->mid); });
            return result;
        }

        /**
         * Append the given link to the path, from the current destination node.
         *
         * @param link Must have the same up node as the current path destination node.
         * @throws std::invalid_argument if `link->up() != destination()`.
         */
        void add(const Link *link) {
            if (link->mup != destination()) {
                throw std::invalid_argument(
                    StrCat(
                        "[OrientedGraphPath::add] Inconsistent path continuation: path-destination=[",
                        destination()->mid,
                        "] link-origin=[",
                        link->mup->mid,
                        "] link-ID=[",
                        link->mid,
                        "]"
                    )
                );
            }
            links_.emplace_back(link);
        }

        /**
         * Append all the given links, in the same order as the input vector.
         */
        void add(const std::vector<const Link *> &links) {
            links_.reserve(links_.size() + links.size());
            for (const Link *link : links) {
                add(link);
            }
        }

        /**
         * Append all the given links, in the reverse order with respect to the input vector.
         */
        void addReversed(const std::vector<const Link *> &links) {
            links_.reserve(links_.size() + links.size());
            for (auto it = links.rbegin(); it != links.rend(); ++it) {
                add(*it);
            }
        }

    };


}
