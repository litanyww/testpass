// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "Steps.h"

#include <sstream>
#include <list>
#include <deque>

class WW::Steps::Impl
{
private:
    typedef std::list<TestStep> stepstore_t;
    typedef std::list<TestStep*> steplist_t;

public:
    Impl()
        : m_allSteps()
        , m_chain() {}
    ~Impl() {}

private: // forbid copy and assignment
    Impl(const Impl& copy);
    Impl& operator=(const Impl& copy);

public:
    stepstore_t& allSteps() { return m_allSteps; }
    const stepstore_t& allSteps() const { return m_allSteps; }
    steplist_t& chain() { return m_chain; }
    const steplist_t& chain() const { return m_chain; }

    std::string debug_dump() const;

private:
    stepstore_t m_allSteps;
    steplist_t m_chain;
};

std::string
WW::Steps::Impl::debug_dump() const
{
    std::ostringstream ost;

    return ost.str();
}

WW::Steps::Steps()
: m_pimpl(new Impl)
{
}

WW::Steps::~Steps()
{
    delete m_pimpl;
}

void
WW::Steps::addStep(const TestStep& step)
{
    m_pimpl->allSteps().push_back(step);
}

std::string
WW::Steps::debug_dump() const
{
    return m_pimpl->debug_dump();
}
