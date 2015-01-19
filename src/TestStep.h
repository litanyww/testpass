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
        typedef Operation<std::string> operation_t;
    public:
        TestStep();
        ~TestStep();
        TestStep(const TestStep& copy);
        TestStep& operator=(const TestStep& copy);
#if __cplusplus >= 201103L
        TestStep(TestStep&& copy);
#endif
    public:
    private:
        operation_t m_operation;
        unsigned int m_cost;
        bool m_required;
        std::string m_description; // describes the steps to take for this test
        // TODO: automation script
    };
}

#endif // INCLUDE_WW_TESTSTEP_HEADER
