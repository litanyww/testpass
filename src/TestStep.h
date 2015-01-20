// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_TESTSTEP_HEADER
#define INCLUDE_WW_TESTSTEP_HEADER

#include "operation.h"

namespace WW
{
    class TestStep
    {
    public:
        typedef Operation<std::string> operation_t;
        typedef operation_t::value_type value_type;
    public:
        TestStep();
        ~TestStep();
        TestStep(const TestStep& copy);
        TestStep& operator=(const TestStep& copy);
#if __cplusplus >= 201103L
        TestStep(TestStep&& copy);
#endif
    public:
        void setDependencies(const value_type& attributes) { m_operation.setDependencies(attributes); }
        void setChanges(const value_type& attributes) { m_operation.setChanges(attributes); }
        void modify(value_type& attributes) const { m_operation.modify(attributes); }
        value_type apply(const value_type& attributes) const { return m_operation.apply(attributes); }
        bool isValid(const value_type& attributes) const { return m_operation.isValid(attributes); }
        bool hasChanges() const { return m_operation.hasChanges(); }

        unsigned int cost() const { return m_cost; }
        unsigned int cost(unsigned int value) { unsigned int result = m_cost; m_cost = value; return result; }

        bool required() const { return m_required; }
        bool required(bool value) { bool result = m_required; m_required = value; return result; }

        const std::string& description() const { return m_description; }
        std::string description(const std::string& value) { std::string result = m_description; m_description = value; return result; }

    private:
        operation_t m_operation;
        unsigned int m_cost;
        bool m_required;
        std::string m_description; // describes the steps to take for this test
        // TODO: automation script
    };
}

#endif // INCLUDE_WW_TESTSTEP_HEADER
