// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_TEST_EXCEPTION_HEADER
#define INCLUDE_WW_TEST_EXCEPTION_HEADER

#include <exception>

namespace WW
{
    class TestException : public std::exception
    {
        char* m_what;
    public:
        ~TestException() throw() { delete m_what; }
        TestException() throw() : m_what(0) {}
        TestException(const char* what) throw(): m_what(0) { assign(what); }
        TestException(const TestException& copy) throw() : m_what(0) { assign(copy.m_what); }
        TestException& operator=(const TestException& copy) throw() {
            if (copy.m_what != m_what) {
                assign(copy.m_what);
            }
            return *this;
        }
    public:
        const char* what() const throw() { return (m_what == 0) ? "" : m_what; }

    private:
        void assign(const char* what) throw() {
            delete m_what;
            unsigned int len;
            for (len = 0 ; what[len] != '\0'; ++len) {}
            m_what = new char[len + 1];
            for (unsigned int i = 0 ; i < len + 1; ++i)
            {
                m_what[i] = what[i];
            }
        }
    };
}

#endif // INCLUDE_WW_TEST_EXCEPTION_HEADER
