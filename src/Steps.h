// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_STEPS_HEADER
#define INCLUDE_WW_STEPS_HEADER

#include "TestStep.h"

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
        typedef size_t size_type; // TODO: there is a better way of doing this.
        typedef value_type& reference;
        typedef value_type* pointer;
        /// typedef xxxx iterator;
        /// typedef xxxx const_iterator;
        /// typedef xxxx reverse_iterator;
        /// typedef xxxx const_reverse_iterator;

    public:
        Steps();
        ~Steps();

    private: // forbid copy and assignment
        Steps(const Steps& copy);
        Steps& operator=(const Steps& copy);

    public:
        void addStep(const TestStep& step);
        void calculate(); // Generate the test pass
        std::string debug_dump() const; // Temporary function which will dump the content to a string

    private:
        class Impl;
        Impl* m_pimpl;
    };
}

#endif // INCLUDE_WW_STEPS_HEADER
