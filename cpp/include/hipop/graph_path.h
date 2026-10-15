#pragma once

#include <hipop/graph.h>
#include <hipop/string_util.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
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

        /**
         * @param links May be empty.
         * @throws std::invalid_argument if `links[i - 1]->down() != links[i]->up()` for some index i,
         *                               or if links is non-empty and `origin != links[0]->up()`.
         */
        OrientedGraphPath(const Node *origin, std::vector<const Link *> links) :
            origin_(origin),
            links_(std::move(links))
        {
            const Node *expectedUpNode = origin_;
            for (const Link *link : links_) {
                if (link->mup != expectedUpNode) {
                    throw std::invalid_argument(
                        StrCat(
                            "[OrientedGraphPath::OrientedGraphPath] Inconsistent path continuation: link-ID=[",
                            link->mid,
                            "] link-origin=[",
                            link->mup->mid,
                            "] current-path-destination=[",
                            expectedUpNode->mid,
                            "]"
                        )
                    );
                }
                expectedUpNode = link->mdown;
            }
        }

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
         * Maximum number of links that can be accomodated in the underlying container
         * without memory re-allocation.
         *
         * This method is provided for optimization purpose only.
         */
        [[nodiscard]] std::size_t capacity() const {
            return links_.capacity();
        }

        /**
         * Increase the capacity of the underlying container so that it can accomodates
         * at least `newCapacity` links without memory re-allocation.
         *
         * This method is provided for optimization purpose only.
         */
        void reserve(std::size_t newCapacity) {
            links_.reserve(newCapacity);
        }

        /**
         * Release the unused memory in the underlying link container.
         *
         * This method is provided for optimization purpose only.
         */
        void shrinkToFit() {
            links_.shrink_to_fit();
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

        class NodeIterator {

            const OrientedGraphPath *path_ = nullptr;
            std::size_t nodeIndex_ = 0;

        public:

            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const Node *;
            using pointer = value_type const *;
            using reference = value_type const &;

            NodeIterator() = default;

            NodeIterator(const OrientedGraphPath *path, std::size_t nodeIndex) :
                path_{ path },
                nodeIndex_{ nodeIndex } {}

            [[nodiscard]] reference operator*() const {
                return nodeIndex_ == 0 ? path_->origin_ : path_->links_[nodeIndex_ - 1]->mdown;
            }

            [[nodiscard]] pointer operator->() const {
                return &operator*();
            }

            NodeIterator &operator++() {
                ++nodeIndex_;
                return *this;
            }

            NodeIterator operator++(int) {
                NodeIterator previous = *this;
                ++nodeIndex_;
                return previous;
            }

            NodeIterator &operator--() {
                --nodeIndex_;
                return *this;
            }

            NodeIterator operator--(int) {
                NodeIterator previous = *this;
                --nodeIndex_;
                return previous;
            }

            [[nodiscard]] bool operator==(const NodeIterator &other) const {
                return path_ == other.path_ && nodeIndex_ == other.nodeIndex_;
            }

            [[nodiscard]] bool operator!=(const NodeIterator &other) const {
                return !operator==(other);
            }
        };

        /**
         * Lightweight view over the nodes of a path, to be used in range-based for-loops.
         */
        class NodeRange {

            const OrientedGraphPath *path_;

        public:

            using iterator = NodeIterator;
            using reverse_iterator = std::reverse_iterator<iterator>;

            explicit NodeRange(const OrientedGraphPath *path) : path_{ path } {}

            [[nodiscard]] bool empty() const {
                return false; // There is always at least 1 node in the path.
            }

            [[nodiscard]] std::size_t size() const {
                return path_->size() + 1; // Number of nodes in the path.
            }

            [[nodiscard]] iterator begin() const {
                return { path_, 0 };
            }

            [[nodiscard]] iterator end() const {
                return { path_, path_->size() + 1 };
            }

            [[nodiscard]] reverse_iterator rbegin() const {
                return reverse_iterator{ end() };
            }

            [[nodiscard]] reverse_iterator rend() const {
                return reverse_iterator{ begin() };
            }
        };

        /**
         * Sequence of nodes visited along the path.
         *
         * @return Guaranteed to be non-empty.
         */
        [[nodiscard]] NodeRange nodes() const {
            return NodeRange{ this };
        }

        /**
         * Sequence of nodes visited along the path, identified by their IDs.
         *
         * @return Guaranteed to be non-empty.
         */
        [[nodiscard]] std::vector<std::string> nodeIds() const {
            std::vector<std::string> result;
            result.reserve(size() + 1);
            for (const Node *node : nodes()) {
                result.emplace_back(node->mid);
            }
            return result;
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
            for (const Link *link : links_) {
                result.emplace_back(link->mid);
            }
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
                        "[OrientedGraphPath::add(const Link *)] ",
                        "Inconsistent path continuation: link-ID=[",
                        link->mid,
                        "] link-origin=[",
                        link->mup->mid,
                        "] path-destination=[",
                        destination()->mid,
                        "]"
                    )
                );
            }
            links_.emplace_back(link);
        }

        /**
         * Append the given path to the current one, from the current destination node.
         *
         * @param other Must have the same origin node as the current path destination node.
         * @throws std::invalid_argument if `other.origin() != destination()`.
         */
        void add(const OrientedGraphPath &other) {
            if (other.origin() != destination()) {
                throw std::invalid_argument(
                    StrCat(
                        "[OrientedGraphPath::add(const OrientedGraphPath &)] ",
                        "Inconsistent path continuation: other-path-origin=[",
                        other.origin()->mid,
                        "] path-destination=[",
                        destination()->mid,
                        "]"
                    )
                );
            }
            std::size_t sz = other.links_.size();
            links_.reserve(links_.size() + sz);
            for (std::size_t i = 0; i < sz; ++i) {
                // WARNING: do not use a foreach loop or a bulk insert here, as that would not work properly
                // if `this` and `other` are the same instance (i.e. path added to itself).
                links_.emplace_back(other.links_[i]);
            }
        }

        /**
         * Extract a sub-path out of the current path.
         *
         * The extracted sub-path starts at the node corresponding to index `pos`,
         * and contains `linkCount` links, or fewer if the current path ends before that.
         *
         * @param pos Must be <= size().
         * @param linkCount If omitted, the returned sub-path extends to the end of the current path.
         */
        [[nodiscard]] OrientedGraphPath subPath(
            std::size_t pos,
            std::size_t linkCount = std::numeric_limits<std::size_t>::max()
        ) const {
            OrientedGraphPath result(node(pos));
            std::size_t newSize = std::min(linkCount, size() - pos);
            result.links_.insert(result.links_.end(), links_.begin() + pos, links_.begin() + pos + newSize);
            return result;
        }

        /**
         * Whether the current path starts with `p` or not.
         */
        [[nodiscard]] bool startsWith(const OrientedGraphPath &p) const {
            std::size_t psz = p.size();
            return psz == 0 ?
                origin() == p.origin() :
                links_.size() >= psz && std::equal(p.links_.begin(), p.links_.end(), links_.begin());
        }

        /**
         * Whether the current path ends with `p` or not.
         */
        [[nodiscard]] bool endsWith(const OrientedGraphPath &p) const {
            std::size_t psz = p.size();
            return psz == 0 ?
                destination() == p.destination() :
                size() >= psz && std::equal(p.links_.begin(), p.links_.end(), links_.begin() + (size() - psz));
        }

    };


    /**
     * Print the path as `from=<origin-ID> links=[<link-ID> <link-ID> ...] to=<destination-ID>`.
     */
    inline std::ostream &operator<<(std::ostream &stream, const OrientedGraphPath &path) {
        stream << "from=" << path.origin()->mid << " links=[";
        bool isFirstLink = true;
        for (const Link *link : path.links()) {
            if (!isFirstLink) {
                stream << " ";
            }
            stream << link->mid;
            isFirstLink = false;
        }
        stream << "] to=" << path.destination()->mid;
        return stream;
    }


} // namespace hipop
