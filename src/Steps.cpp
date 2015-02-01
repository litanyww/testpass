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

#define DEBUG
#define DBGOUT(_x) do { std::cerr << "DEBUG: " << _x << std::endl; } while (0)

typedef WW::TestStep::value_type attributes_t;
typedef std::list<const WW::TestStep*> steplist_t;
typedef std::list<WW::TestStep> stepstore_t;

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

class WW::Steps::Impl
{
private:

public:
    Impl()
        : m_allSteps()
        , m_chain()
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

    void
        applyState(attributes_t& state, const steplist_t& steps)
        {
            for (steplist_t::const_iterator it = steps.begin(); it != steps.end(); ++it) {
#ifdef DEBUG
                if (!(*it)->operation().isValid(state))
                {
                    DBGOUT("ERROR: unexpectedly unable to apply solved state " << *(*it) << " " << (*it)->operation() << " onto " << state);
                    throw WW::TestException("Unable to apply cheapest solution");
                }
#endif
                (*it)->operation().modify(state);
            }
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

    /** solve
     * @params state        starting state
     * @params target       set of desired attributes
     * @params steps        list of available steps
     * @params out_result   results to return
     *
     * Determine the cheapest set of steps to iterate from state to target.  This function will be called recursively
     */
    int
        solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, steplist_t& out_result)
        {
            // DBGOUT("solve(state=" << state << ", target=" << target);
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
            applyState(candidateState, out_result);
            steplist_t otherBits;
            cost += solve(candidateState, target, steps, otherBits);
            out_result.splice(out_result.end(), otherBits);
            return cost;
        }

    // This function is O(log N), and thus will not scale well
    int
        solveAll(const attributes_t& state, const steplist_t& pending, const stepstore_t& steps, steplist_t& out_result, int depth = 0)
        {
            // DBGOUT("(" << depth << ") solveAll(state=" << state << ", pending=" << pending << ", steps, out_result, depth)");
            out_result.clear();
            int cheapest = 0;
            for (steplist_t::const_iterator it = pending.begin(); it != pending.end(); ++it)
            {
                attributes_t newState = state;
                steplist_t solution;
                int cost = solve(state, (*it)->operation().dependencies(), steps, solution);
                solution.push_back(*it);
                cost += (*it)->cost();
                applyState(newState, solution);
                // DBGOUT("(" << depth << ")   first: " << cost << " " << **it << " " << solution);

                if (pending.size() > 1)
                {
                    steplist_t newPending = pending;
                    remove(*it, newPending);

                    steplist_t rest;
                    cost += solveAll(newState, newPending, steps, rest, depth + 1);
                    solution.splice(solution.end(), rest);
                }
                // DBGOUT("(" << depth << ")   full: " << cost << " " << solution);

                if (solution.size() > 0 && (out_result.size() == 0 || cost < cheapest))
                {
                    // DBGOUT("(" << depth << ")  new cheapest " << cost << " " << solution);
                    out_result = solution;
                    cheapest = cost;
                }
            }
            // DBGOUT("(" << depth << ") solved " << cheapest << ": " << out_result);
            return cheapest;
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
}

void
WW::Steps::Impl::calculate()
{
    //DBGOUT("calculate()");

    steplist_t pending;
    clone_required(m_allSteps, pending);

    attributes_t state;
    solveAll(state, pending, m_allSteps, m_chain);
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
