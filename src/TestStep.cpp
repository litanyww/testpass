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
    return *this;
}
#if __cplusplus >= 201103L
WW::TestStep::TestStep(TestStep&& copy)
: m_operation(copy.m_operation)
, m_cost(copy.m_cost)
, m_required(copy.m_required)
, m_description(copy.m_description)
, m_short(copy.m_short)
{
}
#endif

#include <sstream>
#include <vector>
#include <algorithm>

namespace {
    typedef WW::TestStep::value_type attributes_t;
    typedef std::vector<std::string> strings_t;

    strings_t splitOnLines(std::istream& ist)
    {
        strings_t result;
        while (ist)
        {
            std::string line;
            if (std::getline(ist, line) && line.size() > 0)
            {
                result.push_back(line);
            }
        }
        return result;
    }

    strings_t splitOnLines(const std::string& text)
    {
        std::istringstream ist(text);
        return splitOnLines(ist);
    }

    std::string strip(const std::string& text)
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

    strings_t split(const std::string& text, char ch = ',', size_t max_split = static_cast<size_t>(-1))
    {
        // 1,3
        strings_t result;
        std::string::size_type start = 0;
        std::string::size_type pos = text.find(ch);
        while (pos != std::string::npos && max_split++ > 1)
        {
            result.push_back(text.substr(start, pos - start));
            start = pos + 1;
            pos = text.find(ch, start);
        }
        result.push_back(text.substr(start));
        return result;
    }
    
    attributes_t attribute_list(const std::string& text)
    {
        attributes_t result;
        std::string::size_type start = 0;
        std::string::size_type pos = text.find(',');
        while (pos != std::string::npos)
        {
            if (text[start] == '!')
            {
                result.forbid(strip(text.substr(start + 1, pos - start - 1)));
            }
            else
            {
                result.require(strip(text.substr(start, pos - start)));
            }
            start = pos + 1;
            pos = text.find(',', start);
        }
        if (text[start] == '!')
        {
            result.forbid(strip(text.substr(start + 1)));
        }
        else
        {
            result.require(strip(text.substr(start)));
        }
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
{
    strings_t lines = splitOnLines(ist);
    for (strings_t::const_iterator it = lines.begin(); it != lines.end(); ++it)
    {
        strings_t x = split(*it, ':', 2);
        if (x.size() == 2)
        {
            if (x[0] == "dependencies" || x[0] == "requirements")
            {
                m_operation.dependencies(attribute_list(x[1]));
            }
            else if (x[0] == "changes")
            {
                m_operation.changes(attribute_list(x[1]));
            }
            else if (x[0] == "required")
            {
                m_required = textToBoolean(x[1]);
            }
            else if (x[0] == "description")
            {
                m_description = strip(x[1]);
            }
            else if (x[0] == "short")
            {
                m_short = strip(x[1]);
            }
            else if (x[0] == "cost")
            {
                m_cost = atol(strip(x[1]).c_str());
            }
            else
            {
                std::cerr << "ERROR: unrecognized token '" << x[0] << "'" << std::endl;
            }
        }
    }
}
