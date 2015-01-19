// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "operation.h"

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
    op.setDependencies(dependencies);
    op.setChanges(changes);

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
}
