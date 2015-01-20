// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "TestStep.h"

WW::TestStep::TestStep()
: m_operation()
, m_cost(0)
, m_required(false)
, m_description()
{
}

WW::TestStep::~TestStep()
{
}

WW::TestStep::TestStep(const TestStep& copy)
: m_operation(copy.m_operation)
, m_cost(copy.m_cost)
, m_required(copy.m_required)
, m_description(copy.m_description)
{
}

WW::TestStep&
WW::TestStep::operator=(const TestStep& copy)
{
    m_operation = copy.m_operation;
    m_cost = copy.m_cost;
    m_required = copy.m_required;
    m_description = copy.m_description;
    return *this;
}
#if __cplusplus >= 201103L
WW::TestStep::TestStep(TestStep&& copy)
: m_operation(copy.m_operation)
, m_cost(copy.m_cost)
, m_required(copy.m_required)
, m_description(copy.m_description)
{
}
#endif
