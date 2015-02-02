// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "TestStep.h"

#include <sstream>

TEST(TestStep, LoadFromStream)
{
    std::istringstream ist(""
            "short:NiceShortDescription\n"
            "dependencies:installed,onaccess,OAexclusion=/tmp/excluded/eicar.com,haveEicar=/tmp/excluded/eicar.com\n"
            "changes:awesomeness,!fear\n"
            "cost:2\n"
            "required:yes\n"
            "description:Descriptive Long Description\n");

    WW::TestStep step(ist);

    ASSERT_EQ("NiceShortDescription", step.short_desc());
    ASSERT_EQ("Descriptive Long Description", step.description());
    ASSERT_EQ(2, step.cost());
    ASSERT_TRUE(step.required());
    ASSERT_EQ(2, step.operation().changes().size());
    ASSERT_EQ(4, step.operation().dependencies().size());
}

TEST(TestStep, LoadFromStreamWithSpaces)
{
    std::istringstream ist(""
            "short: \tNiceShortDescription \t \n"
            "dependencies: installed , onaccess , OAexclusion=/tmp/excluded/eicar.com , haveEicar=/tmp/excluded/eicar.com \n"
            "changes:  awesomeness  ,  !fear  \n"
            "cost: 2  \n"
            "required:  yes  \n"
            "description:  \tDescriptive Long Description  \t  \n");

    WW::TestStep step(ist);

    ASSERT_EQ("NiceShortDescription", step.short_desc());
    ASSERT_EQ("Descriptive Long Description", step.description());
    ASSERT_EQ(2, step.cost());
    ASSERT_TRUE(step.required());
    ASSERT_EQ(2, step.operation().changes().size());
    ASSERT_EQ(4, step.operation().dependencies().size());
    typedef WW::TestStep::value_type attributes_t;
    const attributes_t& dependencies = step.operation().dependencies();
    for (attributes_t::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
    {
        ASSERT_NE(it->value().c_str()[0], ' ') << "Dependencies should be stripped";
        ASSERT_NE(it->value().c_str()[0], '\t') << "Dependencies should be stripped";
        ASSERT_NE(it->value().c_str()[it->value().size() - 1], ' ') << "Dependencies should be stripped";
        ASSERT_NE(it->value().c_str()[it->value().size() - 1], '\t') << "Dependencies should be stripped";
    }

    const attributes_t& changes = step.operation().changes();
    for (attributes_t::const_iterator it = changes.begin(); it != changes.end(); ++it)
    {
        ASSERT_NE(it->value().c_str()[0], ' ') << "Changes should be stripped";
        ASSERT_NE(it->value().c_str()[0], '\t') << "Changes should be stripped";
        ASSERT_NE(it->value().c_str()[it->value().size() - 1], ' ') << "Changes should be stripped";
        ASSERT_NE(it->value().c_str()[it->value().size() - 1], '\t') << "Changes should be stripped";
    }
}
