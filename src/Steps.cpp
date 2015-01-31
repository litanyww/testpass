// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "Steps.h"

#include "TestException.h"

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

    steplist_t findStepsProviding(const stepstore_t& steps, const attributes_t& attributes)
    {
        steplist_t result;
        for (stepstore_t::const_iterator it = steps.begin(); it != steps.end(); ++it)
        {
            if (it->operation().changes().containsAny(attributes))
            {
                result.push_back(&(*it));
            }
        }
        return result;
    }

    /** solve
     * @params state        starting state
     * @params target       set of desired attributes
     * @params steps        list of available steps
     * @params out_result   results to return
     *
     * Determine the cheapest set of steps to iterate from state to target.  This function will be called recursively
     */
    const int
        solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, steplist_t& out_result)
        {
            DBGOUT("solve(state=" << state << ", target=" << target);
            attributes_t missing;
            state.differences(target, missing);
            if (missing.size() == 0)
            {
                return 0;
            }
            steplist_t candidates = findStepsProviding(steps, missing);
            if (candidates.size() == 0)
            {
                // This one is unusable
                return 9999;
                //throw WW::TestException("No step defined which provides attributes");
            }

            int cost = 9999;
            out_result.clear();
            for (steplist_t::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
            {
                steplist_t list;
                int outcome = 0;
                if ((*it)->operation().isValid(state))
                {
                    // we don't need to search, it is immediately valid
                    list.push_back(*it);
                    outcome = (*it)->cost();
                }
                else
                {
                    outcome = solve(state, (*it)->operation().dependencies(), steps, list);
                    list.push_back(*it);
                }
                if (list.size() > 0 && (out_result.size() == 0 || outcome < cost))
                {
                    cost = outcome;
                    out_result = list;
                }
            }

            if (out_result.size() == 0)
            {
                DBGOUT("No solution for " << state << " to " << target);
                return 0;
            }

            // cheapest should at this point be a sequence starting from
            // `state`, but may not get us all the way to 'target'.  We call
            // this function recursively at this point safely because we can't choose the same path, that set of attributes should already be satisfied.
            
            attributes_t candidateState = state;
            for (steplist_t::const_iterator it = out_result.begin(); it != out_result.end(); ++it)
            {
#ifdef DEBUG
                if (!(*it)->operation().isValid(candidateState))
                {
                    DBGOUT("ERROR: unexpectedly unable to apply solved state " << *(*it) << " onto " << candidateState);
                    throw WW::TestException("Unable to apply cheapest solution");
                }
#endif
                (*it)->operation().modify(candidateState);
            }
            steplist_t otherBits;
            cost += solve(candidateState, target, steps, otherBits);
            out_result.splice(out_result.end(), otherBits);
            return cost;
        }

    void
        remove(const WW::TestStep* item, steplist_t& list)
        {
            for (steplist_t::iterator it = list.begin(); it != list.end(); ++it)
            {
                if (*it == item)
                {
                    list.erase(it);
                    break;
                }
            }
        }

    void
        clone_required(const stepstore_t& allSteps, steplist_t& list)
        {
            list.clear();
            for (stepstore_t::const_iterator it = allSteps.begin(); it != allSteps.end(); ++it)
            {
                const WW::TestStep& step = *it;
                if (step.required())
                {
                    list.push_back(&step);
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

namespace std
{
    std::ostream& operator<<(std::ostream& ost, const steplist_t& list) {
        ost << "[";
        bool comma = false;
        for (steplist_t::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if (comma) {
                ost << ", ";
            }
            else {
                comma = true;
            }
            ost << **it;
        }
        ost << "]";
        return ost;
    }
}

void
WW::Steps::Impl::calculate()
{
    //DBGOUT("calculate()");

    clone_required(m_allSteps, m_pending);
    while (!m_pending.empty())
    {
        const TestStep* candidate = cheapest_next_candidate(m_state, m_pending);

        DBGOUT("resolving: " << *candidate);
        steplist_t solved;
        solve(m_state, candidate->operation().dependencies(), m_allSteps, solved);
        if (solved.size() == 0)
        {
            throw WW::TestException("No solution");
        }
        DBGOUT("  solved dependencies: " << solved);
        for (steplist_t::const_iterator it = solved.begin(); it != solved.end(); ++it)
        {
            (*it)->operation().modify(m_state);
            remove(*it, m_pending);
        }
        m_chain.splice(m_chain.end(), solved);
        remove(candidate, m_pending);
        m_chain.push_back(candidate);
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
