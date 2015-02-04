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

typedef WW::Steps::attributes_t attributes_t;
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
        : m_startState()
        , m_allSteps()
        {}
    ~Impl() {}

private: // forbid copy and assignment
    Impl(const Impl& copy);
    Impl& operator=(const Impl& copy);

public:
    void add(const Steps& steps, bool allAreRequired = false);
    void add(const TestStep& step);
    stepstore_t& allSteps() { return m_allSteps; }
    const stepstore_t& allSteps() const { return m_allSteps; }
    void setState(const attributes_t& state) { m_startState = state; }

    steplist_t calculate(unsigned int complexity) const;

private:
    attributes_t m_startState;
    stepstore_t m_allSteps;
};

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

    // If count is unlimited, this function is O(N!), and thus will not scale well.
    int
        solveAll(const attributes_t& state, const steplist_t& pending, const stepstore_t& steps, steplist_t& out_result, int count = 1)
        {
            // DBGOUT("(" << count << ") solveAll(state=" << state << ", pending=" << pending << ", steps, out_result, count)");
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
                // DBGOUT("(" << count << ")   first: " << cost << " " << **it << " " << solution);

                if (pending.size() > 1 && count > 0)
                {
                    steplist_t newPending = pending;
                    remove(*it, newPending);

                    steplist_t rest;
                    cost += solveAll(newState, newPending, steps, rest, count - 1);
                    solution.splice(solution.end(), rest);
                }
                // DBGOUT("(" << count << ")   full: " << cost << " " << solution);

                if (solution.size() > 0 && (out_result.size() == 0 || cost < cheapest))
                {
                    // DBGOUT("(" << count << ")  new cheapest " << cost << " " << solution);
                    out_result = solution;
                    cheapest = cost;
                }
            }
            // DBGOUT("(" << count << ") solved " << cheapest << ": " << out_result);
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

steplist_t
WW::Steps::Impl::calculate(unsigned int complexity) const
{
    //DBGOUT("calculate()");

    steplist_t pending;
    steplist_t chain;
    clone_required(m_allSteps, pending);

    attributes_t state = m_startState;
    while (pending.size() > 0)
    {
        steplist_t solution;
        solveAll(state, pending, m_allSteps, solution, complexity);
        applyState(state, solution);
        for (steplist_t::const_iterator it = solution.begin(); it != solution.end(); ++it)
        {
            (*it)->operation().modify(state);
            remove(*it, pending);
        }
        chain.splice(chain.end(), solution);
    }
    return chain;
}

void
WW::Steps::Impl::add(const WW::Steps& steps, bool allAreRequired)
{
    const stepstore_t& store = steps.m_pimpl->m_allSteps;
    for (stepstore_t::const_iterator it = store.begin(); it != store.end(); ++it)
    {
        if (allAreRequired) {
            WW::TestStep copy = *it;
            copy.required(true);
            add(copy);
        }
        else
        {
            add(*it);
        }
    }
}

void
WW::Steps::Impl::add(const TestStep& step)
{
    for (stepstore_t::iterator it = m_allSteps.begin(); it != m_allSteps.end(); ++it)
    {
        if (it->short_desc() == step.short_desc())
        {
            m_allSteps.erase(it);
            break;
        }
    }
    m_allSteps.push_back(step);
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
    m_pimpl->add(step);
}

WW::StepList
WW::Steps::calculate(unsigned int complexity) const
{
    steplist_t chain = m_pimpl->calculate(complexity);
    return StepList(chain);
}

void
WW::Steps::add(const Steps& steps)
{
    m_pimpl->add(steps, false);
}

void
WW::Steps::addRequired(const Steps& steps)
{
    m_pimpl->add(steps, true);
}

void
WW::Steps::setState(const attributes_t& state)
{
    m_pimpl->setState(state);
}

WW::StepList
WW::Steps::requiredSteps() const
{
    WW::StepList result;
    for (stepstore_t::const_iterator it = m_pimpl->allSteps().begin(); it != m_pimpl->allSteps().end(); ++it)
    {
        if (it->required()) {
            result.push_back(&(*it));
        }
    }
    return result;
}
