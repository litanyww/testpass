// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include <gtest/gtest.h>

#include "StepList.h"

TEST(TestStepList, TestList)
{
    WW::StepList list;

    ASSERT_EQ(static_cast<size_t>(0), list.size());

    WW::TestStep step;
    step.short_desc("TestCase");
    list.push_back(&step);
    ASSERT_EQ(static_cast<size_t>(1), list.size());
    WW::StepList::const_iterator it = list.begin();
    ASSERT_EQ("TestCase", it->short_desc());
    ASSERT_NE(list.end(), it);
    ++it;
    ASSERT_EQ(list.end(), it);
    --it;
    ASSERT_EQ("TestCase", it->short_desc());
    ASSERT_EQ(list.begin(), it);
}

