// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "operation.h"

#if 0
TEST(TestOperation, ModifyAttributes)
{
    typedef WW::Operation<std::string> operation_t;
    typedef operation_t::value_type attribute_t;

    attribute_t dependencies;
    dependencies.require("one");
    dependencies.require("two");
    dependencies.require("three");

    attribute_t forbidden;
    forbidden.require("four");

    attribute_t added;
    added.require("four");
    added.require("five");

    attribute_t removed;
    removed.require("two");
    removed.require("three");

    operation_t op;
    op.setDependencies(dependencies);
    op.setForbidden(forbidden);
    op.setAdded(added);
    op.setRemoved(removed);

    attribute_t attrs;
    attrs.require("one");
    attrs.require("two");
    attrs.require("three");
    attrs.require("apple");
    attrs.require("banana");
    
    attribute_t expected;
    expected.require("one");
    expected.require("four");
    expected.require("five");
    expected.require("apple");
    expected.require("banana");

    ASSERT_TRUE(op.isValid(attrs)) << "Check operation is valid for these attributes";

    attribute_t modified = op.apply(attrs);
    ASSERT_EQ(expected, modified) << "Check that the attributes were modified correctly";
    ASSERT_FALSE(op.isValid(modified)) << "Attributes are no longer valid after operation was applied";

    modified = attrs;
    op.modify(modified);
    ASSERT_EQ(expected, modified) << "Check that the attributes were modified in-please correctly";
}
#endif
