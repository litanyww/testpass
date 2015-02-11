// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "attributes.h"
#include "TestStep.h"

#include <sstream>

TEST(TestAttributes, AttributeAsSet)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t collection;

    collection.require("one");
    collection.require("two");

    ASSERT_EQ(static_cast<size_t>(2), collection.size()) << "Check the size of the collection";

    collection.erase("one");

    ASSERT_EQ(static_cast<size_t>(1), collection.size()) << "We can erase an element";
    ASSERT_EQ("two", *collection.begin()) << "check that the undeleted item is still there";
    ASSERT_FALSE(collection.begin()->isForbidden()) << "By default, the forbidden flag is unset";

    collection.forbid("two");
    ASSERT_EQ(static_cast<size_t>(1), collection.size()) << "Adding a negative attribute doesn't change the size";
    ASSERT_TRUE(collection.begin()->isForbidden()) << "Make sure that we've changed the forbidden flag";

    collection.require("three");
    collection.require("four");
    std::ostringstream ost;
    ost << collection;
    ASSERT_EQ("four,three,!two", ost.str()) << "Attributes dump to ostream";
}

TEST(TestAttributes, CompoundAttributes)
{
    typedef WW::Attributes<std::string> attribute_t;
    attribute_t collection("one,!one=1,one=2");
    ASSERT_EQ(attribute_t("one=2"), collection);

    collection = attribute_t("two,two=2,two=3,two");
    ASSERT_EQ(attribute_t("two"), collection);

    collection = attribute_t("!two=2,two=2");
    ASSERT_EQ(attribute_t("two=2"), collection);
    
    collection = attribute_t("two=2,!two");
    ASSERT_EQ(attribute_t("!two"), collection);
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
    haystack.require("five=5");

    ASSERT_TRUE(haystack.containsAll(needle)) << "An empty set of attributes always matches";
    ASSERT_FALSE(needle.containsAll(haystack)) << "An empty haystack doesn't match any needle";

    needle.require("two");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on one item";

    needle.require("three");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on two items";

    needle.require("one");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Make sure we match on all items";

    needle.forbid("six");
    ASSERT_TRUE(haystack.containsAll(needle)) << "Check that absent forbidden values are matched";

    needle.require("four");
    ASSERT_FALSE(haystack.containsAll(needle)) << "Checking for a value should not match when the value is forbidden";
    
    needle = attribute_t();
    needle.require("six");
    ASSERT_FALSE(haystack.containsAll(needle)) << "The only item in needle doesn't match";

    needle = attribute_t();
    needle.forbid("one");
    ASSERT_FALSE(haystack.containsAll(needle)) << "Don't match when one of the values is forbidden";

    ASSERT_TRUE(haystack.containsAll(attribute_t("one,five=5,two"))) << "Exact match on attribute with value";
    ASSERT_FALSE(haystack.containsAll(attribute_t("one,five=4,two"))) << "Exact match on attribute with value";
    ASSERT_FALSE(haystack.containsAll(attribute_t("one,five,two"))) << "Exact match on attribute with value";
    ASSERT_FALSE(haystack.containsAll(attribute_t("one=1"))) << "Exact match on attribute with value";
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
    haystack.require("five=5");

    ASSERT_FALSE(haystack.containsAny(needle)) << "An empty set of attributes never matches";
    ASSERT_FALSE(needle.containsAny(haystack)) << "A non-empty needle won't match an empty haystack";

    needle.require("one");

    ASSERT_TRUE(haystack.containsAny(needle)) << "We've matched on only one matching item";

    needle.require("six");
    ASSERT_TRUE(haystack.containsAny(needle)) << "We've matched when only one of the items match";

    needle.forbid("two");
    ASSERT_FALSE(haystack.containsAny(needle)) << "Don't allow any forbidden items, even when we have matches.";

    haystack.forbid("two");
    ASSERT_TRUE(haystack.containsAny(needle)) << "Allow forbidden items if also forbidden in haystack.";

    needle = attribute_t();

    needle.require("six");
    ASSERT_FALSE(haystack.containsAny(needle)) << "Nothing in common, no matches";

    ASSERT_TRUE(haystack.containsAny(attribute_t("five=5"))) << "Exact match on attribute with value";
    ASSERT_FALSE(haystack.containsAny(attribute_t("five=4"))) << "Exact match on attribute with value";
    ASSERT_FALSE(haystack.containsAny(attribute_t("one=1,five"))) << "Exact match on attribute with value";
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
    first.require("peach=tasty");
    first.require("grape=small");

    second.require("one");
    second.forbid("two");
    second.require("three");
    second.require("grape");
    second.forbid("lemon");
    second.require("peach=sour");
    second.forbid("grape");
    
    attribute_t expected;
    expected.require("one");
    expected.require("three");
    expected.require("apple");
    expected.forbid("banana"); // meaningless, but applyChanges didn't change it
    expected.require("peach=sour");

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
    state.require("apple=sweet");

    requirements.require("one");
    requirements.require("deux");
    requirements.forbid("three");
    requirements.forbid("four"); // not in state, so satisfied and not expected in result
    requirements.require("apple=sour");

    attribute_t expected;
    expected.require("deux");
    expected.forbid("three");
    expected.forbid("apple=sweet");
    expected.require("apple=sour");

    attribute_t changes = state.differences(requirements);
    ASSERT_EQ(expected, changes) << "Compare expected differences with reality";
}

TEST(TestAttributes, TestFindChanges)
{
    typedef WW::Attributes<std::string> attribute_t;

    attribute_t required;

    attribute_t::find_changes(attribute_t("one,two,three"), attribute_t("two,!three,four"), required);
    ASSERT_EQ(attribute_t("four,!three"), required);

    attribute_t::find_changes(attribute_t("one=1,two=2,three,four=4,five=5"), attribute_t("two=2,three=3,four=0x04,!five"), required);
    ASSERT_EQ(attribute_t("three=3,four=0x04,!five=5"), required);
}

TEST(TestAttributes, EdgeCases)
{
    typedef WW::Attributes<std::string> attribute_t;

    attribute_t one("one1,one11");
    ASSERT_EQ(static_cast<size_t>(2), one.size());
    attribute_t two("one");
    one.applyChanges(two);
    ASSERT_EQ(attribute_t("one,one1,one11"), one);
}
