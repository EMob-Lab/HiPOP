#include "helpers.h"

#include <hipop/iterator_util.h>

#include <iterator>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using namespace hipop;


namespace {

    struct Person {

        std::string name;
        int age;

        friend bool operator<(const Person &lhs, const Person &rhs) {
            return lhs.name < rhs.name; // Lexicographic order, on name only.
        }
    };

    /**
     * Projection yielding a reference to the name of a person (no copy).
     */
    struct Name {
        const std::string &operator()(const Person &person) const {
            return person.name;
        }
    };

    /**
     * Projection yielding the age of a person (by value).
     */
    struct Age {
        int operator()(const Person &person) const {
            return person.age;
        }
    };

    /**
     * Projection yielding a reference to the age of a person, either read-write or read-only
     * depending on whether `Element` is `Person` or `const Person`.
     */
    template<typename Element>
    struct AgeRef {
        std::conditional_t<std::is_const_v<Element>, const int, int> &operator()(Element &person) const {
            return person.age;
        }
    };

    using NameRange = ProjectingRange<const std::set<Person>, Name>;
    using AgeRange = ProjectingRange<const std::set<Person>, Age>;
    using AgeRefRange = ProjectingRange<std::vector<Person>, AgeRef<Person>>;
    using ConstAgeRefRange = ProjectingRange<const std::vector<Person>, AgeRef<const Person>>;

    std::set<Person> makePeople() {
        return { // Insertion in decreasing age order (i.e. not the same order as induced by `Person::operator<`).
            { "Charlie", 45 },
            { "Alice", 35 },
            { "Bob", 20 },
        };
    }

    /**
     * Values enumerated by the given range, in iteration order (i.e. the order induced by `Person::operator<`).
     */
    template<typename Value, typename Range>
    std::vector<Value> collect(const Range &range) {
        std::vector<Value> result;
        for (const auto &value : range) {
            result.emplace_back(value);
        }
        return result;
    }

} // anonymous namespace


static void test_NameRange() {

    const std::set<Person> people = makePeople();
    NameRange range(people);

    static_assert(std::is_same_v<NameRange::iterator::reference, const std::string &>);
    static_assert(std::is_same_v<NameRange::iterator::value_type, std::string>);

    assertTrue(!range.empty() && range.size() == 3, "[NameRange] Range should have 3 elements");
    assertTrue(
        collect<std::string>(range) == std::vector<std::string>{ "Alice", "Bob", "Charlie" },
        "[NameRange] Range should enumerate the names in the order of the underlying container"
    );
    assertTrue(
        &*range.begin() == &people.begin()->name,
        "[NameRange] Projection returning a reference should not copy the underlying value"
    );
}


static void test_AgeRange() {

    const std::set<Person> people = makePeople();
    AgeRange range(people);

    static_assert(std::is_same_v<AgeRange::iterator::reference, int>);
    static_assert(std::is_same_v<AgeRange::iterator::value_type, int>);

    assertTrue(!range.empty() && range.size() == 3, "[AgeRange] Range should have 3 elements");
    assertTrue(
        collect<int>(range) == std::vector<int>{ 35, 20, 45 },
        "[AgeRange] Range should enumerate the ages in the order of the underlying container"
    );
}


static void test_EmptyRange() {

    const std::set<Person> people;
    NameRange range(people);

    assertTrue(range.empty() && range.size() == 0, "[EmptyRange] Range should be empty");
    assertTrue(range.begin() == range.end(), "[EmptyRange] begin() and end() should be equal");
    assertTrue(collect<std::string>(range).empty(), "[EmptyRange] Range should enumerate nothing");
}


static void test_IteratorOperations() {

    using Iterator = NameRange::iterator;
    static_assert(std::is_default_constructible_v<Iterator>);
    static_assert(std::is_same_v<std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>);

    const std::set<Person> people = makePeople();
    NameRange range(people);

    // Pre-increment.
    Iterator it = range.begin();
    Iterator self = ++it;
    assertTrue(self == it && *it == "Bob", "[IteratorOperations] Pre-increment should return the new position");

    // Post-increment.
    Iterator previous = it++;
    assertTrue(
        *previous == "Bob" && *it == "Charlie",
        "[IteratorOperations] Post-increment should return the previous position"
    );

    // Comparisons.
    assertTrue(
        it != range.begin() && !(it == range.begin()),
        "[IteratorOperations] Different positions should not be equal"
    );
    ++it;
    assertTrue(it == range.end() && !(it != range.end()), "[IteratorOperations] Iterator should reach end()");

    // Standard algorithms.
    assertTrue(std::distance(range.begin(), range.end()) == 3, "[IteratorOperations] std::distance() should work");
}


static void test_LiveView() {

    std::set<Person> people = makePeople();
    const std::set<Person> &cPeople = people;
    NameRange names(cPeople);
    AgeRange ages(cPeople);

    people.insert({ "Aaron", 100 });

    assertTrue(
        collect<std::string>(names) == std::vector<std::string>{ "Aaron", "Alice", "Bob", "Charlie" },
        "[LiveView] Range should reflect the modifications of the underlying container"
    );
    assertTrue(
        collect<int>(ages) == std::vector<int>{ 100, 35, 20, 45 },
        "[LiveView] Several ranges may be built on the same underlying container"
    );

    // Remove people aged < 40.
    for (auto it = people.begin(); it != people.end();) {
        if (it->age < 40) {
            it = people.erase(it);
        }
        else {
            ++it;
        }
    }

    assertTrue(
        names.size() == 2 && collect<std::string>(names) == std::vector<std::string>{ "Aaron", "Charlie" },
        "[LiveView] Range should reflect the removals in the underlying container"
    );
    assertTrue(
        ages.size() == 2 && collect<int>(ages) == std::vector<int>{ 100, 45 },
        "[LiveView] All the ranges built on the same underlying container should reflect the removals"
    );
}


static void test_ReadWriteRange() {

    std::vector<Person> people{ { "Alice", 35 }, { "Bob", 20 } };
    const std::vector<Person> &cPeople = people;
    AgeRefRange range(people);
    ConstAgeRefRange cRange(cPeople);

    static_assert(std::is_same_v<AgeRefRange::iterator::reference, int &>);
    static_assert(std::is_same_v<ConstAgeRefRange::iterator::reference, const int &>);
    static_assert(std::is_same_v<AgeRefRange::iterator::value_type, int>);
    static_assert(std::is_same_v<ConstAgeRefRange::iterator::value_type, int>);

    // Modification through the read-write range.
    for (int &age : range) {
        age += 100;
    }
    assertTrue(
        people[0].age == 135 && people[1].age == 120,
        "[ReadWriteRange] Modification through the read-write range should be visible in the underlying container"
    );
    assertTrue(
        collect<int>(cRange) == std::vector<int>{ 135, 120 },
        "[ReadWriteRange] Modification through the read-write range should be visible in the read-only range"
    );
}


static void test_Conversions() {

    // Read-write -> read-only only.
    static_assert(std::is_convertible_v<AgeRefRange, ConstAgeRefRange>);
    static_assert(!std::is_convertible_v<ConstAgeRefRange, AgeRefRange>);
    static_assert(std::is_convertible_v<AgeRefRange::iterator, ConstAgeRefRange::iterator>);
    static_assert(!std::is_convertible_v<ConstAgeRefRange::iterator, AgeRefRange::iterator>);

    // No conversion between ranges/iterators yielding different things out of the same container.
    static_assert(!std::is_convertible_v<NameRange, AgeRange>);
    static_assert(!std::is_convertible_v<NameRange::iterator, AgeRange::iterator>);

    std::vector<Person> people{ { "Alice", 35 }, { "Bob", 20 } };
    const std::vector<Person> &cPeople = people;
    AgeRefRange range(people);

    ConstAgeRefRange converted = range;
    assertTrue(
        converted.size() == 2 && collect<int>(converted) == std::vector<int>{ 35, 20 },
        "[Conversions] Converted range should enumerate the same values"
    );

    ConstAgeRefRange::iterator cit = range.begin();
    AgeRefRange::iterator it = range.begin();
    assertTrue(cit == it && it == cit && !(cit != it) && !(it != cit), "[Conversions] Mixed comparisons should work");
    assertTrue(
        range.end() == ConstAgeRefRange(cPeople).end(),
        "[Conversions] Read-only and read-write ranges should have the same end"
    );
}


int testProjectionRange(int, char**) {
    test_NameRange();
    test_AgeRange();
    test_EmptyRange();
    test_IteratorOperations();
    test_LiveView();
    test_ReadWriteRange();
    test_Conversions();
    return 0;
}
