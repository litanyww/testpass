// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "TestStep.h"
#include "Steps.h"
#include "TestException.h"

#include <sstream>

TEST(TestStep, LoadFromStream)
{
    std::istringstream ist(""
            "short:NiceShortDescription\n"
            "dependencies:installed,onaccess,OAexclusion=/tmp/excluded/eicar.com,haveEicar=/tmp/excluded/eicar.com\n"
            "changes:awesomeness,!fear\n"
            "cost:2\n"
            "required:yes\n"
            "description:Descriptive Long Description\n"
            "script:echo \"Hello, World!\"\n");


    WW::TestStep step(ist);

    ASSERT_EQ("NiceShortDescription", step.short_desc());
    ASSERT_EQ("Descriptive Long Description", step.description());
    ASSERT_EQ("echo \"Hello, World!\"", step.script());
    ASSERT_EQ(static_cast<unsigned int>(2), step.cost());
    ASSERT_TRUE(step.required());
    ASSERT_EQ(static_cast<size_t>(2), step.operation().changes().size());
    ASSERT_EQ(static_cast<size_t>(4), step.operation().dependencies().size());
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
    ASSERT_EQ(static_cast<unsigned int>(2), step.cost());
    ASSERT_TRUE(step.required());
    ASSERT_EQ(static_cast<size_t>(2), step.operation().changes().size());
    ASSERT_EQ(static_cast<size_t>(4), step.operation().dependencies().size());
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

TEST(TestStep, MultiLineDescription)
{
    std::string description(
            "A double colon indicates a multi-line entry\n"
            "  which is terminated by an empty line\n"
            "\n"
            "which means a blank line is acceptable"
            );
    std::istringstream ist(std::string() +
            "short:NiceShortDescription\n"
            "description::\n" + description + "\n"
            ".\n"
            "dependencies:one,two,three\n"
            "changes:!three,four\n"
            "cost:3\n"
            "required:yes\n");

    WW::TestStep step(ist);

    ASSERT_EQ("NiceShortDescription", step.short_desc());
    ASSERT_EQ(description, step.description());
    ASSERT_EQ(static_cast<unsigned int>(3), step.cost());
}

TEST(TestStep, TestSolve)
{
    WW::Steps steps;
    steps.setShowProgress(false);
    steps.addStep(WW::TestStep(
                "short: one\n"
                "dependencies: two\n"
                "cost: 1\n"
                "required: yes\n"
                "description: one\n"));
    steps.addStep(WW::TestStep(
                "short: three\n"
                "dependencies: banana\n"
                "changes: three\n"
                "cost: 3\n"
                "required: no\n"
                "description: three\n"));
    steps.addStep(WW::TestStep(
                "short: two\n"
                "changes: two\n"
                "dependencies: three\n"
                "required: no\n"
                "cost: 2\n"
                "description: two\n"));
    WW::StepList solution = steps.calculate();
    ASSERT_EQ(static_cast<size_t>(0), solution.size());

    steps.setState(WW::TestStep::attributes_t("banana"));
    solution = steps.calculate();
    EXPECT_EQ(static_cast<size_t>(3), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("three", it->short_desc());
    ++it;
    ASSERT_EQ("two", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
}

TEST(TestStep, TestSolve2)
{
    WW::Steps steps;
    steps.setShowProgress(false);
    steps.addStep(WW::TestStep(
                "short: one\n"
                "dependencies: two,three\n"
                "changes: !three\n"
                "cost: 1\n"
                "required: yes\n"
                "description: one\n"));
    steps.addStep(WW::TestStep(
                "short: three\n"
                "dependencies: banana\n"
                "changes: three\n"
                "cost: 3\n"
                "required: no\n"
                "description: three\n"));
    steps.addStep(WW::TestStep(
                "short: two\n"
                "changes: two,!three\n"
                "dependencies: three\n"
                "required: no\n"
                "cost: 2\n"
                "description: two\n"));

    steps.setState(WW::TestStep::attributes_t("banana"));
    WW::StepList solution = steps.calculate();
    EXPECT_EQ(static_cast<size_t>(4), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("three", it->short_desc()) << "two depends on three";
    ++it;
    ASSERT_EQ("two", it->short_desc()) << "two unsets three";
    ++it;
    ASSERT_EQ("three", it->short_desc()) << "three is required again because two unset it";
    ++it;
    ASSERT_EQ("one", it->short_desc()) << "complete";
}
