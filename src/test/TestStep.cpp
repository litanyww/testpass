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
            "dependencies:one,!two,fruit=banana,!hat=trilby\n"
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
    ASSERT_EQ(WW::TestStep::value_type("awesomeness,!fear"), step.operation().changes());
    ASSERT_EQ(static_cast<size_t>(4), step.operation().dependencies().size());
    ASSERT_EQ(WW::TestStep::value_type("one,!two,fruit=banana,!hat=trilby"), step.operation().dependencies());
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
    ASSERT_EQ(static_cast<size_t>(3), solution.size());
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
    ASSERT_EQ(static_cast<size_t>(4), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("three", it->short_desc()) << "two depends on three";
    ++it;
    ASSERT_EQ("two", it->short_desc()) << "two unsets three";
    ++it;
    ASSERT_EQ("three", it->short_desc()) << "three is required again because two unset it";
    ++it;
    ASSERT_EQ("one", it->short_desc()) << "complete";
}

TEST(TestStep, MultiplexRequired)
{
    WW::Steps steps;
    steps.setShowProgress(false);
    steps.addStep(WW::TestStep(
                "short: one\n"
                "dependencies: two,three,four\n"
                "changes: !three\n"
                "cost: 1\n"
                "required: yes\n"
                "description: one\n"));
    steps.addStep(WW::TestStep(
                "short: two_one\n"
                "changes: two=1\n"
                "cost: 1\n"
                "required: no\n"
                "description: two\n"));
    steps.addStep(WW::TestStep(
                "short: two_two\n"
                "changes: two=2\n"
                "cost: 1\n"
                "required: no\n"
                "description: two\n"));
    steps.addStep(WW::TestStep(
                "short: three_one\n"
                "changes: three=1\n"
                "cost: 1\n"
                "required: no\n"
                "description: three\n"));
    steps.addStep(WW::TestStep(
                "short: three_two\n"
                "changes: three=2\n"
                "cost: 1\n"
                "required: no\n"
                "description: three\n"));
    steps.addStep(WW::TestStep(
                "short: four\n"
                "changes: four\n"
                "cost: 1\n"
                "required: no\n"
                "description: four\n"));

    WW::StepList solution = steps.calculate();
    ASSERT_EQ(static_cast<size_t>(11), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("two_two", it->short_desc());
    ++it;
    ASSERT_EQ("three_two", it->short_desc());
    ++it;
    ASSERT_EQ("four", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
    ++it;
    ASSERT_EQ("three_one", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
    ++it;
    ASSERT_EQ("two_one", it->short_desc());
    ++it;
    ASSERT_EQ("three_two", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
    ++it;
    ASSERT_EQ("three_one", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
}

TEST(TestStep, TestOptimised)
{
    WW::Steps steps;
    steps.setShowProgress(false);
    steps.addStep(WW::TestStep(
                "short: work\n"
                "dependencies: prep\n"
                "changes: !prep\n"
                "cost: 1\n"
                "required: yes\n"
                "description: one\n"));
    // Now the non-required steps we'll need to solve this
    steps.addStep(WW::TestStep(
                "short: simple\n"
                "changes: prep\n"
                "cost: 3\n"
                "required: no\n"
                "description: simple is more expensive long term, cheaper in the short term\n"));
    steps.addStep(WW::TestStep(
                "short: optimal\n"
                "dependencies: setup\n"
                "changes: prep\n"
                "cost: 1\n"
                "required: no\n"
                "description: optimal is more expensive long term, cheaper in the short term\n"));
    steps.addStep(WW::TestStep(
                "short: setup\n"
                "changes: setup\n"
                "cost: 3\n"
                "required: no\n"
                "description: this step makes 'apple2' more expensive than 'apple1' the first time it is used\n"));

    WW::StepList solution = steps.calculate();
    EXPECT_EQ(static_cast<size_t>(2), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("simple", it->short_desc()) << "Currently the simplest is best";
    ++it;
    ASSERT_EQ("work", it->short_desc()) << "three is required again because two unset it";

    steps.addStep(WW::TestStep(
                "short: moreWork\n"
                "dependencies: prep\n"
                "changes: !prep\n"
                "cost: 1\n"
                "required: yes\n"
                "description: two\n"));

    solution = steps.calculate();
    EXPECT_EQ(static_cast<size_t>(5), solution.size());
    it = solution.begin();
    ASSERT_EQ("setup", it->short_desc()) << "It is now best to eat the expensive setup cost because it makes the subsequent steps cheaper";
    ++it;
    ASSERT_EQ("optimal", it->short_desc()) << "We can now use the optimal prep solution";
    ++it;
    ASSERT_EQ("moreWork", it->short_desc()) << "three is required again because two unset it";
    ++it;
    ASSERT_EQ("optimal", it->short_desc()) << "Again we can use the optimal prep";
    ++it;
    ASSERT_EQ("work", it->short_desc());

    steps.addStep(WW::TestStep(
                "short: evenMoreWork\n"
                "dependencies: prep\n"
                "changes: !prep\n"
                "cost: 1\n"
                "required: yes\n"
                "description: three\n"));

    solution = steps.calculate();
    EXPECT_EQ(static_cast<size_t>(7), solution.size());
    it = solution.begin();
    ASSERT_EQ("setup", it->short_desc()) << "two depends on three";
    ++it;
    ASSERT_EQ("optimal", it->short_desc()) << "two unsets three";
    ++it;
    ASSERT_EQ("evenMoreWork", it->short_desc()) << "three is required again because two unset it";
    ++it;
    ASSERT_EQ("optimal", it->short_desc()) << "complete";
    ++it;
    ASSERT_EQ("moreWork", it->short_desc()) << "three is required again because two unset it";
    ++it;
    ASSERT_EQ("optimal", it->short_desc()) << "complete";
    ++it;
    ASSERT_EQ("work", it->short_desc()) << "three is required again because two unset it";
}

TEST(TestStep, TestMultipleCompoundDeps)
{
    WW::Steps steps;
    steps.setShowProgress(false);
    steps.addStep( // addStep now takes stream or string, since it needs to multiplex steps
                "short: one\n"
                "dependencies: two=apple,two=banana\n"
                "cost: 1\n"
                "required: yes\n"
                "description: one\n");
    steps.addStep( // addStep now takes stream or string, since it needs to multiplex steps
                "short: apple\n"
                "changes: two=apple\n"
                "cost: 2\n"
                "required: no\n"
                "description: apple\n");
    steps.addStep( // addStep now takes stream or string, since it needs to multiplex steps
                "short: banana\n"
                "changes: two=banana\n"
                "cost: 3\n"
                "required: no\n"
                "description: banana\n");
    steps.addStep( // addStep now takes stream or string, since it needs to multiplex steps
                "short: pear\n"
                "changes: two=pear\n"
                "cost: 1\n"
                "required: no\n"
                "description: pear\n");

    WW::StepList solution = steps.calculate();
    ASSERT_EQ(static_cast<size_t>(4), solution.size());
    WW::StepList::const_iterator it = solution.begin();
    ASSERT_EQ("banana", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
    ++it;
    ASSERT_EQ("apple", it->short_desc());
    ++it;
    ASSERT_EQ("one", it->short_desc());
    ++it;
}
