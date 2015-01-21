// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "attributes.h"

#include <sstream>

TEST(TestAttributes, AttributeAsSet)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t collection;

    collection.require("one");
    collection.require("two");

    ASSERT_EQ(2, collection.size()) << "Check the size of the collection";

    collection.erase("one");

    ASSERT_EQ(1, collection.size()) << "We can erase an element";
    ASSERT_EQ("two", *collection.begin()) << "check that the undeleted item is still there";
    ASSERT_FALSE(collection.begin()->isForbidden()) << "By default, the forbidden flag is unset";

    collection.forbid("two");
    ASSERT_EQ(1, collection.size()) << "Adding a negative attribute doesn't change the size";
    ASSERT_TRUE(collection.begin()->isForbidden()) << "Make sure that we've changed the forbidden flag";

    collection.require("three");
    collection.require("four");
    std::ostringstream ost;
    ost << collection;
    ASSERT_EQ("[four,three,!two]", ost.str()) << "Attributes dump to ostream";
}

TEST(TestAttributes, InsertUsingIterators)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;
    attribute_t second;

    first.require("one");
    first.require("two");

    second.require("one");
    second.forbid("two");

    ASSERT_NE(second, first) << "The isForbidden flag makes them different";
    first.insert(second.begin(), second.end());
    ASSERT_EQ(second, first) << "Insert using iterators updates the isForbidden flag on matching attributes";
}

TEST(TestAttributes, AttributeComparison)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;
    attribute_t same;
    attribute_t different;

    ASSERT_EQ(first, same) << "Empty attributes are always the same";

    first.require("one");
    first.require("two");

    ASSERT_NE(first, same) << "Empty and non-empty attributes are not the same";

    same.require("two");
    same.require("one");

    different.require("one");
    different.require("two."); // <- see; different!

    ASSERT_EQ(first, same) << "Check that we can compare collections, and that the order of element insertion doesn't matter";
    ASSERT_NE(first, different) << "Check that we don't always say that they're the same";

    different.erase("two.");
    different.forbid("two"); // forbidden
    ASSERT_NE(first, different) << "Check that forbidden values are different to required";
}

TEST(TestAttributes, Copying)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;

    first.require("one");
    first.forbid("two");

    attribute_t second(first);
    attribute_t third;

    ASSERT_EQ(first, second) << "We can copy collections";
    ASSERT_NE(second, third) << "And compare them";
    third = first;
    ASSERT_EQ(second, third) << "Assignment works too";
}

TEST(TestAttributes, TestIterators)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;

    first.require("1. one");
    first.require("2. two");
    
    attribute_t::const_iterator it = first.begin();
    ASSERT_EQ("1. one", *it) << "begin() will return iterator to first value ordered alphabetically";
    ++it;
    ASSERT_EQ("2. two", *it) << "after increment, we have the second value";
    ++it;
    ASSERT_EQ(first.end(), it) << "at end of iterator";
}

TEST(TestAttributes, TestReverseIterators)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;

    first.require("1. one");
    first.require("2. two");

    attribute_t::const_reverse_iterator it = first.rbegin();
    ASSERT_EQ("2. two", *it) << "rbegin() returns the last value ordered alphabetically";
    ++it;
    ASSERT_EQ("1. one", *it) << "after increment, it returns the previous";
    ++it;
    ASSERT_EQ(first.rend(), it) << "at end of reverse iterator";
}

TEST(TestAttributes, TestContainsAll)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t haystack;
    attribute_t needle;

    ASSERT_TRUE(haystack.containsAll(needle)) << "An empty set of attributes always matches";

    haystack.require("one");
    haystack.require("two");
    haystack.require("three");
    haystack.forbid("four"); // a forbidden value

    ASSERT_TRUE(haystack.containsAll(needle)) << "An empty set of attributes always matches";
    ASSERT_FALSE(needle.containsAll(haystack)) << "An empty haystack doesn't match any needle";

    needle.require("two");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on one item";

    needle.require("three");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on two items";

    needle.require("one");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on all items";

    needle.forbid("five");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Check that absent forbidden values are matched";

    needle.require("four");
    ASSERT_FALSE(haystack.containsAll(needle)) << "Checking for a value should not match when the value is forbidden";
    
    needle = attribute_t();
    needle.require("five");
    ASSERT_FALSE(haystack.containsAll(needle)) << "The only item in needle doesn't match";

    needle = attribute_t();
    needle.forbid("one");
    ASSERT_FALSE(haystack.containsAll(needle)) << "Don't match when one of the values is forbidden";
}

TEST(TestAttributes, TestContainsAny)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t haystack;
    attribute_t needle;

    ASSERT_FALSE(haystack.containsAny(needle)) << "An empty set of attributes never matches";

    haystack.require("one");
    haystack.require("two");
    haystack.require("three");
    haystack.forbid("four");

    ASSERT_FALSE(haystack.containsAny(needle)) << "An empty set of attributes never matches";
    ASSERT_FALSE(needle.containsAny(haystack)) << "A non-empty needle won't match an empty haystack";

    needle.require("one");

    ASSERT_TRUE(haystack.containsAny(needle)) << "We've matched on only one matching item";

    needle.require("five");
    ASSERT_TRUE(haystack.containsAny(needle)) << "We've matched when only one of the items match";

    needle.forbid("two");
    ASSERT_FALSE(haystack.containsAny(needle)) << "Don't allow any forbidden items, even when we have matches.";

    needle = attribute_t();

    needle.require("five");
    ASSERT_FALSE(haystack.containsAny(needle)) << "Nothing in common, no matches";
}

TEST(TestAttributes, TestApplyChanges)
{
    // current state should never contain forbidden items, since they're
    // meaningless.  Their presence isn't illegal; they just don't do much.
    // Operation requirements on the other hand have both requirements and forbidden items.  They're also easy to understand.
    // Operation changes treats require items as items to add, and forbid items as items to remove.
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t first;
    attribute_t second;

    first.require("one");
    first.require("two");
    first.forbid("three");
    first.require("apple");
    first.forbid("banana");

    second.require("one");
    second.forbid("two");
    second.require("three");
    second.require("grape");
    second.forbid("lemon");
    
    attribute_t expected;
    expected.require("one");
    expected.require("three");
    expected.require("apple");
    expected.forbid("banana"); // meaningless, but applyChanges didn't change it
    expected.require("grape");

    first.applyChanges(second);
    ASSERT_EQ(expected, first) << "testing apply";
}

TEST(TestAttributes, TestDifferences)
{
    // I have a current state represented as an Attributes objet.  I also have
    // an Operation object with a set of requirements.  I want to know which
    // requirements are missing, and which are present but need to be removed.
    // Basically, if I can find an operation which has a set of changes which
    // exactly match the generated attributes, then applying that operation
    // would satisfy the requirements of the first operation.

    typedef WW::Attributes<std::string> attribute_t;
    attribute_t state;
    attribute_t requirements;

    state.require("one");
    state.require("two");
    state.require("three");

    requirements.require("one");
    requirements.require("deux");
    requirements.forbid("three");

    attribute_t expected;
    expected.require("deux");
    expected.forbid("three");

    attribute_t changes = state.differences(requirements);
    ASSERT_EQ(expected, changes) << "Compare expected differences with reality";
}
