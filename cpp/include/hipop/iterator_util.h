#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>


namespace hipop {


    /**
     * Forward iterator wrapping an iterator on an underlying container, so as to hide this container,
     * and to transform the value type yielded by the iterator.
     *
     * The value yielded by the iterator is `Projection{}(*baseIterator)`.
     *
     * @tparam BaseIterator Iterator on the underlying container (at least a forward iterator).
     * @tparam Projection Default-constructible callable object with the following signature: `Y(X element)`,
     *                    where `X` is the reference type of `BaseIterator` (i.e. a reference to an element
     *                    of the underlying container), and `Y` the reference type yielded by the iterator
     *                    (or value type if returning a reference is not possible/suitable).
     */
    template<typename BaseIterator, typename Projection>
    class ProjectingIterator {

        BaseIterator it_{};

        template<typename, typename>
        friend class ProjectingIterator;

    public:

        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using reference = std::invoke_result_t<Projection, typename std::iterator_traits<BaseIterator>::reference>;
        using value_type = std::remove_cv_t<std::remove_reference_t<reference>>;
        using pointer = void;

        ProjectingIterator() = default;

        explicit ProjectingIterator(BaseIterator it) : it_(std::move(it)) {}

        /**
         * Implicit conversion from the read-write iterator to the read-only one
         * (enabled if both the underlying iterators and the yielded values are convertible).
         */
        template<typename OtherBaseIterator, typename OtherProjection, typename = std::enable_if_t<
            std::is_convertible_v<OtherBaseIterator, BaseIterator> &&
            !std::is_same_v<OtherBaseIterator, BaseIterator> &&
            std::is_convertible_v<
                typename ProjectingIterator<OtherBaseIterator, OtherProjection>::reference,
                reference
            >
        >>
        ProjectingIterator(const ProjectingIterator<OtherBaseIterator, OtherProjection> &other) : it_(other.it_) {}

        [[nodiscard]] reference operator*() const {
            return Projection{}(*it_);
        }

        ProjectingIterator &operator++() {
            ++it_;
            return *this;
        }

        ProjectingIterator operator++(int) {
            ProjectingIterator result = *this;
            ++it_;
            return result;
        }

        [[nodiscard]] friend bool operator==(const ProjectingIterator &lhs, const ProjectingIterator &rhs) {
            return lhs.it_ == rhs.it_;
        }

        [[nodiscard]] friend bool operator!=(const ProjectingIterator &lhs, const ProjectingIterator &rhs) {
            return lhs.it_ != rhs.it_;
        }
    };


    /**
     * Lightweight view of an underlying container, so as to hide this container.
     *
     * The range does not own the container: the container must outlive the range.
     *
     * @tparam Container Type of the underlying container, possibly `const`-qualified to get a read-only view.
     * @tparam Projection Same as for `ProjectingIterator`.
     */
    template<typename Container, typename Projection>
    class ProjectingRange {

        Container *container_;

        template<typename, typename>
        friend class ProjectingRange;

    public:

        using iterator = ProjectingIterator<decltype(std::declval<Container &>().begin()), Projection>;

        explicit ProjectingRange(Container &container) : container_(&container) {}

        /**
         * Implicit conversion from the read-write range to the read-only one
         * (enabled if the corresponding iterators are convertible).
         */
        template<typename OtherContainer, typename OtherProjection, typename = std::enable_if_t<
            std::is_same_v<const OtherContainer, Container> &&
            !std::is_same_v<OtherContainer, Container> &&
            std::is_convertible_v<typename ProjectingRange<OtherContainer, OtherProjection>::iterator, iterator>
        >>
        ProjectingRange(const ProjectingRange<OtherContainer, OtherProjection> &other) : container_(other.container_) {}

        [[nodiscard]] bool empty() const {
            return container_->empty();
        }

        [[nodiscard]] std::size_t size() const {
            return container_->size();
        }

        [[nodiscard]] iterator begin() const {
            return iterator(container_->begin());
        }

        [[nodiscard]] iterator end() const {
            return iterator(container_->end());
        }
    };


} // namespace hipop
