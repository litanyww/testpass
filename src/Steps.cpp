// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "Steps.h"

#include <sstream>
#include <list>
#include <deque>
#include <iostream>

#define DBGOUT(_x) do { std::cerr << "DEBUG: " << _x << std::endl; } while (0)

typedef WW::TestStep::value_type attributes_t;
typedef std::list<const WW::TestStep*> steplist_t;
typedef std::list<WW::TestStep> stepstore_t;

class WW::Steps::Impl
{
private:

public:
    Impl()
        : m_allSteps()
        , m_chain()
        , m_pending()
        , m_state()
        {}
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
    void calculate();

private:
    stepstore_t m_allSteps;
    steplist_t m_chain;
    mutable steplist_t m_pending; // steps which still need to be scheduled
    attributes_t m_state;
};

std::string
WW::Steps::Impl::debug_dump() const
{
    std::ostringstream ost;
    unsigned int item = 0;

    for (steplist_t::const_iterator it = m_chain.begin(); it != m_chain.end(); ++it)
    {
        ost << ++item << ". " << (*it)->description() << std::endl;
    }

    return ost.str();
}

namespace {
    const WW::TestStep*
        cheapest_next_candidate(const attributes_t& state, const steplist_t& pending)
        {
            const WW::TestStep* result = 0;
            unsigned int fewest_attributes = 0;
            for (steplist_t::const_iterator it = pending.begin(); it != pending.end(); ++it)
            {
                if (!(*it)->required())
                {
                    continue;
                }

                attributes_t diffs = (*it)->operation().getDifferences(state);
                size_t count = diffs.size();
                if (result == 0)
                {
                    result = *it;
                    fewest_attributes = count;
                    continue;
                }
                else if (count <= fewest_attributes)
                {
                    unsigned int cost = (*it)->cost();

                    if (count < fewest_attributes || cost < result->cost())
                    {
                        result = *it;
                        fewest_attributes = count;
                    }
                }
            }
            return result;
        }

    const WW::TestStep*
        navigate_to(const attributes_t& target, const attributes_t& state, const steplist_t& steps, bool required = false)
        {
            const WW::TestStep* result = 0;
            unsigned int fewest_attributes = 0;
            for (steplist_t::const_iterator it = steps.begin(); it != steps.end(); ++it)
            {
                if (!(*it)->operation().hasChanges() || !(*it)->operation().isValid(state) || (required && !(*it)->required()))
                {
                    continue;
                }
                attributes_t nextState = (*it)->operation().apply(state);
                attributes_t diffs = target.differences(nextState);
                size_t count = diffs.size();
                if (result == 0)
                {
                    result = *it;
                    fewest_attributes = count;
                    continue;
                }
                else if (count <= fewest_attributes)
                {
                    unsigned int cost = (*it)->cost();

                    if (count < fewest_attributes || cost < result->cost())
                    {
                        result = *it;
                        fewest_attributes = count;
                    }
                }
            }
            return result;
        }
    void
        remove(const WW::TestStep* item, steplist_t& pending)
        {
            for (steplist_t::iterator it = pending.begin(); it != pending.end(); ++it)
            {
                if (*it == item)
                {
                    pending.erase(it);
                    break;
                }
            }
        }
    void
        clone(const stepstore_t& allSteps, steplist_t& list)
        {
            list.clear();
            for (stepstore_t::const_iterator it = allSteps.begin(); it != allSteps.end(); ++it)
            {
                const WW::TestStep& step = *it;
                list.push_back(&step);
            }
        }
}

void
WW::Steps::Impl::calculate()
{
    DBGOUT("calculate()");

    clone(m_allSteps, m_pending);
    while (!m_pending.empty())
    {
        const TestStep* candidate = cheapest_next_candidate(m_state, m_pending);
        if (candidate == 0)
        {
            DBGOUT("No pending required candidate exists which is also valid");
            // No pending required candidate exists which is also valid
            break;
        }
        while (!candidate->operation().isValid(m_state))
        {
            steplist_t available;
            clone(m_allSteps, available);

            // If we can, see if we can use one of our pending, required steps.
            const TestStep* step = navigate_to(candidate->operation().dependencies(), m_state, m_pending, true);
            if (step != 0)
            {
                remove(step, m_pending);
            }
            else
            {
                step = navigate_to(candidate->operation().dependencies(), m_state, available);
                if (step == 0)
                {
                    std::cerr << "ERROR: Unable to work out how to satisfy dependencies: " << candidate->operation().dependencies() << std::endl;
                    return;
                }
            }
            DBGOUT("navigating to " << *step);

            m_chain.push_back(step);
            remove(step, m_pending);
            step->operation().modify(m_state);
        }
        DBGOUT("cheapest candidate=" << *candidate);
        m_chain.push_back(candidate);
        remove(candidate, m_pending);
        candidate->operation().modify(m_state);
    }
}

///
///
///

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

void
WW::Steps::calculate()
{
    m_pimpl->calculate();
}
