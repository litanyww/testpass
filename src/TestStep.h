// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_TESTSTEP_HEADER
#define INCLUDE_WW_TESTSTEP_HEADER

#include "operation.h"

#include <string>
#include <iostream>

namespace WW
{
    class TestStep
    {
    public:
        typedef Operation<std::string> operation_t;
        typedef operation_t::value_type value_type;
        typedef typename operation_t::size_type size_type;
    public:
        TestStep();
        ~TestStep();
        TestStep(std::istream& ist);
        TestStep(const TestStep& copy);
        TestStep& operator=(const TestStep& copy);
#if __cplusplus >= 201103L
        TestStep(TestStep&& copy);
#endif
    public:
        void dependencies(const value_type& attributes) { m_operation.dependencies(attributes); }
        void changes(const value_type& attributes) { m_operation.changes(attributes); }
        const operation_t& operation() const { return m_operation; }

        unsigned int cost() const { return m_cost; }
        unsigned int cost(unsigned int value) { unsigned int result = m_cost; m_cost = value; return result; }

        bool required() const { return m_required; }
        bool required(bool value) { bool result = m_required; m_required = value; return result; }

        const std::string& description() const { return m_description; }
        std::string description(const std::string& value) { std::string result = m_description; m_description = value; return result; }

        const std::string& short_desc() const { return m_short; }
        std::string short_desc(const std::string& value) { std::string result = m_short; m_short = value; return result; }

        const std::string& script() const { return m_script; }
        std::string script(const std::string& value) { std::string result = m_script; m_script = value; return result; }

    private:
        operation_t m_operation;
        unsigned int m_cost;
        bool m_required;
        std::string m_description; // describes the steps to take for this test
        std::string m_short; // short description
        std::string m_script;
        // TODO: automation script
    };

    template <class Stream>
        Stream&
        operator<<(Stream& str, const TestStep& ob)
        {
            str << "[" << ob.short_desc();
            if (ob.required())
            {
                str << "*";
            }
            if (ob.cost() != 0)
            {
                str << " (" << ob.cost() << ")";
            }
            str << "]";
            return str;
        }
}

#endif // INCLUDE_WW_TESTSTEP_HEADER
