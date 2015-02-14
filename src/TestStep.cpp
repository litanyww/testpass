// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "TestStep.h"

#include "Steps.h"

WW::TestStep::TestStep()
: m_operation()
, m_cost(0)
, m_required(false)
, m_description()
, m_short()
, m_script()
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
, m_short(copy.m_short)
, m_script(copy.m_script)
{
}

WW::TestStep&
WW::TestStep::operator=(const TestStep& copy)
{
    m_operation = copy.m_operation;
    m_cost = copy.m_cost;
    m_required = copy.m_required;
    m_description = copy.m_description;
    m_short = copy.m_short;
    m_script = copy.m_script;
    return *this;
}
#if __cplusplus >= 201103L
WW::TestStep::TestStep(TestStep&& copy)
: m_operation(copy.m_operation)
, m_cost(copy.m_cost)
, m_required(copy.m_required)
, m_description(copy.m_description)
, m_short(copy.m_short)
, m_script(copy.m_script)
{
}
#endif

#include <sstream>
#include <vector>
#include <algorithm>

WW::TestStep::TestStep(const std::string& text)
: m_operation()
, m_cost(0)
, m_required(true)
, m_description()
, m_short()
, m_script()
{
    std::istringstream ist(text);
    Steps steps(ist);
    if (steps.size() != 0) {
        *this = steps.front();
    }
}

WW::TestStep::TestStep(std::istream& ist)
: m_operation()
, m_cost(0)
, m_required(true)
, m_description()
, m_short()
, m_script()
{
    Steps steps(ist);
    if (steps.size() != 0) {
        *this = steps.front();
    }
}

std::string
WW::TestStep::strip(const std::string& text)
{
    std::string::size_type start = text.find_first_not_of("\r\n\t ");
    if (start == std::string::npos)
    {
        return std::string();
    }
    std::string::size_type end = text.find_last_not_of("\r\n\t ");
    // 0^2
    return text.substr(start, 1 + end - start);
}

