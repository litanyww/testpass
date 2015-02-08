// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "operation.h"
#include "TestStep.h"

#include <sstream>

TEST(TestOperation, ModifyAttributes)
{
    typedef WW::Operation<std::string> operation_t;
    typedef operation_t::value_type attribute_t;

    attribute_t dependencies;
    dependencies.require("one");
    dependencies.require("two");
    dependencies.require("three");
    dependencies.forbid("four");

    attribute_t changes;
    changes.require("four");
    changes.require("five");
    changes.forbid("two");
    changes.forbid("three");

    operation_t op;
    op.dependencies(dependencies);
    ASSERT_FALSE(op.hasChanges()) << "This operation has no changes";
    op.changes(changes);
    ASSERT_TRUE(op.hasChanges()) << "This operation now has changes";

    attribute_t state;
    state.require("one");
    state.require("two");
    state.require("apple");
    state.require("banana");

    ASSERT_FALSE(op.isValid(state)) << "We're missing 'three'";
    state.require("three");
    state.require("four");
    ASSERT_FALSE(op.isValid(state)) << "State has forbidden element 'four'";
    state.erase("four");

    ASSERT_TRUE(op.isValid(state)) << "Check operation is valid for these attributes";
    
    attribute_t expected;
    expected.require("one");
    expected.require("four");
    expected.require("five");
    expected.require("apple");
    expected.require("banana");

    attribute_t modified = op.apply(state);

    ASSERT_EQ(expected, modified) << "Check that the attributes were modified correctly";
    ASSERT_FALSE(op.isValid(modified)) << "State is no longer valid after operation was applied";

    op.modify(state);
    ASSERT_EQ(expected, state) << "Check that the attributes were modified in-please correctly";

    std::ostringstream ost;
    ost << op;
    ASSERT_EQ("[requirement:!four,one,three,two,changes:five,four,!three,!two]", ost.str()) << "Operation can be streamed";
}

TEST(TestOperation, GetDifferences)
{
    typedef WW::Operation<std::string> operation_t;
    typedef operation_t::value_type attribute_t;

    attribute_t state;
    state.require("apple");
    state.require("one");
    state.require("two"); // oops, the operation forbids this attribute
    state.require("four");

    attribute_t dependencies;
    dependencies.require("one"); // already matched
    dependencies.forbid("two"); 
    dependencies.require("three"); // state doesn't have this

    operation_t op;
    op.dependencies(dependencies);

    attribute_t expected;
    expected.require("three");
    expected.forbid("two");

    ASSERT_EQ(expected, op.getDifferences(state)) << "Get a set of changes which would have to be applied to state to match requirements";
}

TEST(TestOperation, TestComplexForbid)
{
    typedef WW::Operation<std::string> operation_t;
    typedef operation_t::value_type attributes_t;

    attributes_t state("installed=candidate,onaccess");
    attributes_t changes("!installed");
    attributes_t expected("onaccess");

    operation_t op;
    op.changes(changes);
    op.modify(state);

    ASSERT_EQ(expected, state) << "Compound attributes are not removed as expected";
}
