// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_STEPS_HEADER
#define INCLUDE_WW_STEPS_HEADER

#include "TestStep.h"
#include "StepList.h"

namespace WW
{
    class Steps
    {
        // TODO: It would be nice to make this class confirm to the list
        // iterator API; but define its own iterator classes.  This would allow
        // it to iterate internally over a list containing pointers, but to
        // have its own iterator reference the actual contents; to dereference
        // the pointer.  This is worth thinking about because the interface to
        // this class doesn't use pointers.
        typedef TestStep value_type;
        typedef value_type::size_type size_type;
        typedef value_type& reference;
        typedef value_type* pointer;
    public:
        typedef TestStep::value_type attributes_t;

    public:
        Steps();
        ~Steps();

    private: // forbid copy and assignment
        Steps(const Steps& copy);
        Steps& operator=(const Steps& copy);

    public:
        void add(const Steps& steps);
        void addRequired(const Steps& steps);
        void markNotRequired(const std::string& short_desc);
        void addStep(const TestStep& step);
        void setState(const attributes_t& state);
        StepList calculate() const; // Generate the test pass
        StepList requiredSteps() const;
        const TestStep* step(const std::string& short_desc) const;

    private:
        class Impl;
        Impl* m_pimpl;
    };
}

#endif // INCLUDE_WW_STEPS_HEADER
