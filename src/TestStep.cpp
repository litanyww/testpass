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

namespace {
    typedef WW::TestStep::value_type attributes_t;
    typedef std::vector<std::string> strings_t;

    strings_t split(const std::string& text, char ch = ',', size_t max_split = static_cast<size_t>(-1))
    {
        // 1,3
        strings_t result;
        std::string::size_type start = 0;
        std::string::size_type pos = text.find(ch);
        while (pos != std::string::npos && max_split-- > 1)
        {
            result.push_back(text.substr(start, pos - start));
            start = pos + 1;
            pos = text.find(ch, start);
        }
        result.push_back(text.substr(start));
        return result;
    }

    std::string toLower(const std::string& text)
    {
        std::string result = text;
        std::transform(result.begin(), result.begin(), result.end(), tolower);
        return result;
    }

    bool textToBoolean(const std::string& text)
    {
        std::string lcText(toLower(text));
        return text == "1" || text == "true" || text == "yes";
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
    std::string line;
    while (ist) {
        std::getline(ist, line);
        strings_t x = split(line, ':', 2);
        if (x.size() == 2)
        {
            std::string key(x[0]);
            std::string value(strip(x[1]));

            if (value == ":")
            {
                value.clear();
                while (ist) {
                    std::getline(ist, line);
                    if (strip(line) == ".") {
                        break;
                    }
                    if (value.size() > 0) {
                        value.append("\n");
                    }
                    if (!strip(line).empty()) {
                        value.append(line);
                    }
                }
            }
            if (key == "dependencies" || key == "requirements")
            {
                m_operation.dependencies(attributes_t(value));
            }
            else if (key == "changes")
            {
                m_operation.changes(attributes_t(value));
            }
            else if (key == "required")
            {
                m_required = textToBoolean(strip(value));
            }
            else if (key == "description")
            {
                m_description = strip(value);
            }
            else if (key == "short")
            {
                m_short = strip(value);
            }
            else if (key == "script")
            {
                m_script = strip(value);
            }
            else if (key == "cost")
            {
                m_cost = atol(strip(value).c_str());
            }
            else
            {
                std::cerr << "ERROR: unrecognized token '" << key << "'" << std::endl;
            }
        }
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

