// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "Steps.h"

#include "StepList.h"
#include "TestException.h"

#include <sstream>
#include <deque>
#include <iostream>
#include <iomanip>

#include <time.h>

#define DEBUG
#define DBGOUT(_x) do { std::cerr << "DEBUG: " << _x << std::endl; } while (0)

typedef WW::Steps::attributes_t attributes_t;
typedef std::list<WW::TestStep> stepstore_t;

inline attributes_t operator+(const attributes_t& lhs, const attributes_t& rhs) {
    attributes_t result = lhs;
    result.insert(rhs.begin(), rhs.end());
    return result;
}

namespace std
{
    std::ostream& operator<<(std::ostream& ost, const WW::StepList& list) {
        ost << "[";
        bool comma = false;
        for (WW::StepList::const_iterator it = list.begin(); it != list.end(); ++it)
        {
            if (comma) {
                ost << ", ";
            }
            else {
                comma = true;
            }
            ost << *it;
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

    WW::StepList calculate() const;

private:
    attributes_t m_startState;
    stepstore_t m_allSteps;
};

namespace {

    WW::StepList findStepsProviding(const stepstore_t& steps, const attributes_t& attributes)
    {
        WW::StepList result;
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
        applyState(attributes_t& state, const WW::StepList& steps)
        {
            for (WW::StepList::const_iterator it = steps.begin(); it != steps.end(); ++it) {
#ifdef DEBUG
                if (!it->operation().isValid(state))
                {
                    std::ostringstream ost;

                    attributes_t cr;

                    attributes_t::find_changes(state, it->operation().dependencies(), cr);

                    ost << "ERROR: unexpectedly unable to apply solved state " << *it << " " << it->operation() << " onto " << state << ".  Missing " << cr;
                    throw WW::TestException(ost.str().c_str());
                }
#endif
                it->operation().modify(state);
            }
        }

    int solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, WW::StepList& out_result);
    int
        solveOrThrow(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, WW::StepList& out_result)
        {
            int cost = solve(state, target, steps, out_result);
            if (cost > 0 && out_result.empty()) {
                attributes_t cr;

                attributes_t::find_changes(state, target, cr);

                std::ostringstream ost;
                ost << "No solution, need these dependencies defined: " << cr << " to get from " << state << " to " << target;
                throw WW::TestException(ost.str().c_str());
            }
            return cost;
        }

    /** solve
     * @params state        starting state
     * @params target       set of desired attributes
     * @params steps        list of available steps
     * @params out_result   results to return

     * Determine the cheapest set of steps to iterate from state to target.  This function will be called recursively
     */
    int
        solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, WW::StepList& out_result)
        {
            out_result.clear();
            // DBGOUT("solve(state=" << state << ", target=" << target);
            attributes_t changes_required;
            attributes_t::find_changes(state, target, changes_required);
            if (changes_required.size() == 0)
            {
                return 0;
            }
            WW::StepList candidates = findStepsProviding(steps, changes_required);
            if (candidates.size() == 0)
            {
                // This one is unusable
                std::ostringstream ost;
                ost << "No step defined which provides attributes " << changes_required;
                return 99999;
            }

            int cost = 0;
            bool solved = false;
            attributes_t missing_attributes;
            for (WW::StepList::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
            {
                WW::StepList list;
                int outcome = 0;
                if (it->operation().isValid(state))
                {
                    // we don't need to search, it is immediately valid
                    outcome = it->cost();
                }
                else
                {
                    outcome = it->cost() + solve(state, it->operation().dependencies(), steps, list);
                    if (outcome > 0 && list.empty()) {
                        // No solution was found
                        attributes_t cd;
                        attributes_t::find_changes(state, it->operation().dependencies(), cd);
                        missing_attributes.insert(cd.begin(), cd.end());
                        continue;
                    }
                }
                list.push_back(&(*it));
                if (list.size() > 0 && (out_result.size() == 0 || outcome < cost))
                {
                    solved = true;
                    cost = outcome;
                    out_result = list;
                }
            }

            if (!solved) {
                std::ostringstream ost;
                ost << "No solution for " << changes_required << ", missing attributes: " << missing_attributes;
                // throw WW::TestException(ost.str().c_str());
                return 1;
            }

            if (out_result.empty())
            {
                return 1;
            }

            // cheapest should at this point be a sequence starting from
            // `state`, but may not get us all the way to 'target'.  We call
            // this function recursively at this point safely because we can't choose the same path, that set of attributes should already be satisfied.

            attributes_t candidateState = state;
            applyState(candidateState, out_result);
            WW::StepList otherBits;
            int solveCost = solve(candidateState, target, steps, otherBits);
            if (solveCost > 0 && otherBits.empty()) {
                // This solution doesn't work.
                out_result.clear();
                return 1;
            }
            cost += solveCost;
            out_result.splice(out_result.end(), otherBits);
            return cost;
        }

    void
        append(WW::StepList& dst, const WW::StepList& src) {
            for (WW::StepList::const_iterator it = src.begin(); it != src.end(); ++it) {
                dst.push_back(&(*it));
            }
        }

    int
        solveForSequence(const attributes_t& startState, WW::StepList::const_iterator begin, WW::StepList::const_iterator end, const stepstore_t& steps, WW::StepList& out_result)
        {
            // DBGOUT("(" << count << ") solveAll(state=" << state << ", pending=" << pending << ", steps, out_result, count)");
            out_result.clear();
            attributes_t state = startState;
            int cost = 0;
            for (WW::StepList::const_iterator it = begin; it != end; ++it) {
                WW::StepList solution;
                int item_cost = solve(state, it->operation().dependencies(), steps, solution);
                if (solution.size() > 0)
                {
                    cost += item_cost;
                    applyState(state, solution);
                    append(out_result, solution);
                }
                else if (item_cost > 0) {
                    return 0; // empty solution means failure
                }
                cost += it->cost();
                it->operation().modify(state);
                out_result.push_back(&(*it));
            }
            return cost;
        }

    WW::StepList::iterator
        bestInsertionPoint(const attributes_t& startState, WW::StepList& sequence, const WW::TestStep& step, const stepstore_t& steps)
        {
            // std::list::insert() requires a non-const iterator (fixed in
            // C++11), which means this function must return a non-const
            // iterator, which in turn means that sequence must be non-const.
            // Please fix when moving to C++11.
            // DBGOUT("bestInsertionPoint(startState, sequence=" << sequence << ", step=" << step << ", steps)");
            WW::StepList solution;
            attributes_t accumulated_state = startState;
            WW::StepList::iterator insert_before = sequence.end();
            int cheapest = 0;
            int accumulated_cost = 0;

            // We're going to run through the sequence from start to end,
            // changing nothing.  We just remember and subsequently return the
            // best insertion point.

            for (WW::StepList::iterator it = sequence.begin(); it != sequence.end(); ++it) {
                attributes_t state = accumulated_state;
                int cost = accumulated_cost + solve(state, step.operation().dependencies(), steps, solution);
                if (cost == 0 || !solution.empty()) {
                    applyState(state, solution);
                    cost += step.cost();
                    step.operation().modify(state);
                    cost += solveForSequence(state, it, sequence.end(), steps, solution);
                    if (!solution.empty() && (insert_before == sequence.end() || cost < cheapest)) {
                        cheapest = cost;
                        insert_before = it;
                    }
                }
                {
                    int cost = solveOrThrow(accumulated_state, it->operation().dependencies(), steps, solution);
                    accumulated_cost += cost + it->cost();
                }
                applyState(accumulated_state, solution);
                it->operation().modify(accumulated_state);
            }
            // We finally get to work out whether the best insertion point is right at the end.
            {
                int cost = solve(accumulated_state, step.operation().dependencies(), steps, solution);
                if (cost == 0 || !solution.empty()) {
                    accumulated_cost += cost + step.cost();
                    if (accumulated_cost < cheapest) {
                        insert_before = sequence.end();
                    }
                }
            }
            return insert_before;
        }

    int
        solveAll(const attributes_t& state, const WW::StepList& pending, const stepstore_t& steps, WW::StepList& out_result)
        {
            WW::StepList order;

            std::cerr << "Compiling:    ";
            unsigned int count = pending.size();
            unsigned int item = 0;
            for (WW::StepList::const_iterator it = pending.begin(); it != pending.end(); ++it)
            {
                unsigned int percent = item++ * 100 / count;
                std::cerr << "\b\b\b" << std::setw(2) << percent << "%";
                try {
                    WW::StepList::iterator insert_point = bestInsertionPoint(state, order, *it, steps);
                    order.insert(insert_point, *it);
                }
                catch (...) {
                    std::cerr << std::endl;
                    throw;
                }
            }
            std::cerr << "\b\b\bdone!" << std::endl;
            return solveForSequence(state, order.begin(), order.end(), steps, out_result);
        }

    void
        clone_required(const stepstore_t& allSteps, WW::StepList& list)
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

WW::StepList
WW::Steps::Impl::calculate() const
{
    //DBGOUT("calculate()");

    StepList pending;
    StepList chain;
    clone_required(m_allSteps, pending);

    attributes_t state = m_startState;
    solveAll(state, pending, m_allSteps, chain);
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
WW::Steps::calculate() const
{
    StepList chain = m_pimpl->calculate();
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
    StepList result;
    for (stepstore_t::const_iterator it = m_pimpl->allSteps().begin(); it != m_pimpl->allSteps().end(); ++it)
    {
        if (it->required()) {
            result.push_back(&(*it));
        }
    }
    return result;
}

void
WW::Steps::markNotRequired(const std::string& short_desc)
{
    for (stepstore_t::iterator it = m_pimpl->allSteps().begin(); it != m_pimpl->allSteps().end(); ++it)
    {
        if (it->short_desc() == short_desc)
        {
            it->required(false);
            break;
        }
    }
}

const WW::TestStep*
WW::Steps::step(const std::string& short_desc) const
{
    const stepstore_t& store = m_pimpl->allSteps();

    for (stepstore_t::const_iterator it = store.begin(); it != store.end(); ++it) {
        if (it->short_desc() == short_desc) {
            return &(*it);
        }
    }
    return 0;
}
