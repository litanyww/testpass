// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#include "Steps.h"

#include "StepList.h"
#include "TestException.h"

#include <deque>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <map>
#include <sstream>

#include <time.h>

#define DEBUG
#define DBGOUT(_x) do { std::cerr << "DEBUG: " << _x << std::endl; } while (0)

typedef WW::Steps::attributes_t attributes_t;
typedef attributes_t::value_type::value_type string_t;
typedef std::list<WW::TestStep> stepstore_t;
typedef std::set<string_t> compound_attributes_t;
typedef std::map<string_t, compound_attributes_t> compound_map_t;
typedef std::list<attributes_t> att_list_t;

template <typename Stream>
Stream& operator<<(Stream& str, const compound_attributes_t& ob) {
    str << "[";
    bool comma = false;
    for (compound_attributes_t::const_iterator it = ob.begin(); it != ob.end(); ++it) {
        if (comma) {
            str << ", ";
        }
        else {
            comma = true;
        }
        str << *it;
    }
    str << "]";
    return str;
}

template <typename Stream>
Stream& operator<<(Stream& str, const compound_map_t& ob) {
    str << "[";
    bool comma = false;
    for (compound_map_t::const_iterator it = ob.begin(); it != ob.end(); ++it) {
        if (comma) {
            str << ", ";
        }
        else {
            comma = true;
        }
        str << it->first << " => " << it->second;
    }
    str << "]";
    return str;
}

template <typename Stream>
Stream& operator<<(Stream& str, const att_list_t& ob) {
    str << "[";
    bool comma = false;
    for (att_list_t::const_iterator it = ob.begin(); it != ob.end(); ++it) {
        if (comma) {
            str << ", ";
        }
        else {
            comma = true;
        }
        str << *it;
    }
    str << "]";
    return str;
}

class WW::Steps::Impl
{
private:

public:
    Impl()
        : m_startState()
        , m_allSteps()
        , m_showProgress(true)
        {}
    ~Impl() {}

private: // forbid copy and assignment
    Impl(const Impl& copy);
    Impl& operator=(const Impl& copy);

public:
    void add(const Steps& steps, bool allAreRequired = false);
    void add(const TestStep& step);
    void add(std::istream& str);
    stepstore_t& allSteps() { return m_allSteps; }
    const stepstore_t& allSteps() const { return m_allSteps; }
    void setState(const attributes_t& state) { m_startState = state; }
    void setShowProgress(bool showProgress) { m_showProgress = showProgress; }

    WW::StepList calculate() const;

private:
    attributes_t m_startState;
    stepstore_t m_allSteps;
    bool m_showProgress;
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

    int solveForSequence(const attributes_t& startState, WW::StepList::const_iterator begin, WW::StepList::const_iterator end, const stepstore_t& steps, WW::StepList& out_result, bool scanToEnd = false);

    /** solve
     * @params state        starting state
     * @params target       set of desired attributes
     * @params steps        list of available steps
     * @params out_result   results to return

     * Determine the cheapest set of steps to iterate from state to target.  This function will be called recursively
     */
    int
        solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, WW::StepList& out_result, WW::StepList::const_iterator chainStart, WW::StepList::const_iterator chainEnd)
        {
            out_result.clear();
            // DBGOUT("solve(state=" << state << ", target=" << target << ", steps, out_result, chainStart, chainEnd) " << WW::StepList(chainStart, chainEnd));
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
                    outcome = it->cost() + solve(state, it->operation().dependencies(), steps, list, chainStart, chainEnd);
                    if (outcome > 0 && list.empty()) {
                        // No solution was found
                        attributes_t cd;
                        attributes_t::find_changes(state, it->operation().dependencies(), cd);
                        missing_attributes.insert(cd.begin(), cd.end());
                        continue;
                    }
                }
                list.push_back(&(*it));

                if (list.size() > 0) {
                    if (chainStart != chainEnd) {
                        // This isn't working because we are calculating the *dependencies* - we don't know the item to solve.  Can't do this here.
                        attributes_t copy = state;
                        applyState(copy, list);
                        if (chainStart->operation().isValid(copy)) {
                            // DBGOUT("  Solving remaining chain - cost=" << cost << ": " << list);
                            WW::StepList tmp;
                            cost += solveForSequence(copy, chainStart, chainEnd, steps, tmp, true);
                            // DBGOUT("  Solved remaining chain - cost=" << cost << ": " << (list + tmp));
                            // We want to see whether the solution satisfies target.
                            // If it does, and if we have access to a list of remaining
                            // elements, then we want to solve that list, to see
                            // whether we can introduce a rule which will make that
                            // subsequent chain even cheaper.  That remaining chain
                            // need not be a full list; it could be just a fixed count,
                            // an optimisation which ought to reduce the performance
                            // overhead.

                            // This function will all solveForSequence(), while that function
                            // is still expected to call this one.  We need to limit
                            // when this recursion happens, since in some cases we do
                            // want it and in others it just isn't useful - or may even
                            // be harmful.
                            //
                        }
                    }

                    if (list.size() > 0 && (out_result.size() == 0 || outcome < cost))
                    {
                        solved = true;
                        cost = outcome;
                        out_result = list;
                    }
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
            // DBGOUT("  solved: " << cost << ": " << out_result);
            return cost;
        }

    int
        solve(const attributes_t& state, const attributes_t& target, const stepstore_t& steps, WW::StepList& out_result)
        {
            static WW::StepList dummy;
            return solve(state, target, steps, out_result, dummy.end(), dummy.end());
        }

    void
        append(WW::StepList& dst, const WW::StepList& src) {
            for (WW::StepList::const_iterator it = src.begin(); it != src.end(); ++it) {
                dst.push_back(&(*it));
            }
        }

    int
        solveForSequence(const attributes_t& startState, WW::StepList::const_iterator begin, WW::StepList::const_iterator end, const stepstore_t& steps, WW::StepList& out_result, bool scanToEnd)
        {
            // DBGOUT("solveForSequence(state=" << startState << ", begin=" << *begin << ", end, steps, out_result, scanToEnd=" << scanToEnd << ") " << WW::StepList(begin, end));
            out_result.clear();
            attributes_t state = startState;
            int cost = 0;
            WW::StepList::const_iterator scanEnd = begin;
            for (int i = 0 ; (i < 15) && scanEnd != end ; ++i) {
                ++scanEnd;
            }
            for (WW::StepList::const_iterator it = begin; it != end; ++it) {
                if (scanEnd != end) {
                    ++scanEnd;
                }
                WW::StepList solution;
                int item_cost = solve(state, it->operation().dependencies(), steps, solution, (scanToEnd ? it : scanEnd), scanEnd);
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
        solveAll(const attributes_t& state, const WW::StepList& pending, const stepstore_t& steps, WW::StepList& out_result, bool showProgress = true)
        {
            WW::StepList order;

            if (showProgress) {
                std::cerr << "Compiling:    ";
            }
            unsigned int count = pending.size();
            unsigned int item = 0;
            for (WW::StepList::const_iterator it = pending.begin(); it != pending.end(); ++it)
            {
                unsigned int percent = item++ * 100 / count;
                if (showProgress) {
                    std::cerr << "\b\b\b" << std::setw(2) << percent << "%";
                }
                try {
                    WW::StepList::iterator insert_point = bestInsertionPoint(state, order, *it, steps);
                    order.insert(insert_point, *it);
                }
                catch (...) {
                    if (showProgress) {
                        std::cerr << std::endl;
                    }
                    throw;
                }
            }
            if (showProgress) {
                std::cerr << "\b\b\bdone!" << std::endl;
            }
            return solveForSequence(state, order.begin(), order.end(), steps, out_result, true);
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

    compound_map_t
        getCompoundAttributes(const stepstore_t& store)
        {
            compound_map_t result;
            for (stepstore_t::const_iterator it_step = store.begin(); it_step != store.end(); ++it_step) {
                const attributes_t& changes = it_step->operation().changes();
                for (attributes_t::const_iterator it_change = changes.begin(); it_change != changes.end(); ++it_change) {
                    const string_t& change = it_change->value();
                    string_t::size_type pos = change.find('=');
                    if (pos != string_t::npos) {
                        string_t key = change.substr(0, pos);
                        string_t value = change.substr(pos + 1);
                        compound_attributes_t& item = result[key];
                        item.insert(value);
                    }
                }
            }
            return result;
        }

    void
        transferCompoundSteps(stepstore_t& store, stepstore_t& compound, const compound_map_t& map)
        {
            stepstore_t::iterator it = store.begin();
            while (it != store.end()) {
                const attributes_t& deps = it->operation().dependencies();
                if (deps.empty()) {
                    ++it;
                    continue;
                }
                stepstore_t::iterator next = it;
                ++next;
                for (attributes_t::const_iterator it_deps = deps.begin(); it_deps != deps.end(); ++it_deps) {
                    const string_t& deps = it_deps->value();
                    string_t::size_type pos = deps.find('=');
                    if (pos == string_t::npos) {
                        if (map.find(deps) != map.end()) {
                            compound.push_back(*it);
                            next = store.erase(it);
                            break;
                        }
                    }
                }
                it = next;
            }
        }

    att_list_t
        multiplexAttributes(const string_t& key, const compound_attributes_t& values, const att_list_t& src)
        {
            att_list_t result;

            for (att_list_t::const_iterator it = src.begin(); it != src.end(); ++it) {
                attributes_t attr = *it; // make a copy
                attr.erase(key);
                for (compound_attributes_t::const_iterator it = values.begin(); it != values.end(); ++it) {
                    attributes_t copy(attr);
                    copy.insert(key + "=" + *it);
                    result.push_back(copy);
                }
            }
            return result;
        }

    attributes_t
        onlyCompoundAttributes(const attributes_t& atts, const compound_map_t& map)
        {
            attributes_t result;
            for (attributes_t::const_iterator it = atts.begin(); it != atts.end(); ++it) {
                if (map.find(it->value()) != map.end()) {
                    result.insert(*it);
                }
            }
            return result;
        }

    void
        expandStep(stepstore_t& store, const WW::TestStep& step, const compound_map_t& cmap)
        {
            att_list_t deps;
            attributes_t attributes = step.operation().dependencies();
            deps.push_back(attributes);
            attributes_t compound_atts = onlyCompoundAttributes(attributes, cmap);

            for (attributes_t::const_iterator it = compound_atts.begin(); it != compound_atts.end(); ++it)
            {
                const string_t& value = it->value();
                compound_map_t::const_iterator m_it = cmap.find(value);
                if (m_it != cmap.end()) {
                    const compound_attributes_t& comps = m_it->second;
                    deps = multiplexAttributes(((it->isForbidden()) ? "!" + it->key() : it->key()), comps, deps);
                }
            }
            for (att_list_t::const_iterator it = deps.begin(); it != deps.end(); ++it) {
                WW::TestStep copy = step;
                copy.dependencies(*it);
                store.push_back(copy);
            }
        }

    void
        expandCompoundAttributes(stepstore_t& store)
        {
            stepstore_t compound_steps;
            compound_map_t map = getCompoundAttributes(store);
            transferCompoundSteps(store, compound_steps, map);
            for (stepstore_t::const_iterator it = compound_steps.begin(); it != compound_steps.end(); ++it) {
                expandStep(store, *it, map);
            }
        }
}

WW::StepList
WW::Steps::Impl::calculate() const
{
    //DBGOUT("calculate()");

    StepList pending;
    StepList chain;
    stepstore_t steps = m_allSteps;

    expandCompoundAttributes(steps);
    clone_required(steps, pending);

    attributes_t state = m_startState;
    solveAll(state, pending, m_allSteps, chain, m_showProgress);
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
        if (it->short_desc() == step.short_desc() && it->operation().dependencies() == step.operation().dependencies())
        {
            m_allSteps.erase(it);
            break;
        }
    }
    m_allSteps.push_back(step);
}

void
WW::Steps::Impl::add(std::istream& str)
{
    typedef std::list<attributes_t::value_type> _att_t;
    _att_t dependencies;
    attributes_t changes;
    std::string description;
    std::string short_desc;
    std::string script;
    int cost = 0;
    bool isRequired = false;

    std::string line;
    while (str) {
        std::getline(str, line);
        strings_t x = split(line, ':', 2);
        if (x.size() == 2)
        {
            std::string key(x[0]);
            std::string value(strip(x[1]));

            if (value == ":")
            {
                value.clear();
                while (str) {
                    std::getline(str, line);
                    if (strip(line) == ".") {
                        break;
                    }
                    if (value.size() > 0) {
                        value.append("\n");
                    }
                    if (!strip(line).empty()) {
                        value.append(line);
                    }
                }
            }
            if (key == "dependencies" || key == "requirements")
            {
                std::string::size_type start = 0;
                std::string::size_type pos = value.find(',');
                while (pos != std::string::npos)
                {
                    dependencies.push_back(strip(value.substr(start, pos - start)));
                    start = pos + 1;
                    pos = value.find(',', start);
                }
                dependencies.push_back(strip(value.substr(start)));
            }
            else if (key == "changes")
            {
                changes = attributes_t(value);
            }
            else if (key == "required")
            {
                isRequired = textToBoolean(strip(value));
            }
            else if (key == "description")
            {
                description = strip(value);
            }
            else if (key == "short")
            {
                short_desc = strip(value);
            }
            else if (key == "script")
            {
                script = strip(value);
            }
            else if (key == "cost")
            {
                cost = atol(strip(value).c_str());
            }
            else
            {
                std::cerr << "ERROR: unrecognized token '" << key << "'" << std::endl;
            }
        }
    }
    // dependencies contains something like [[one],[fruit=apple],[fruit=pear][dog=corgi][dog=spaniel]]
    // the above will turn into four steps
    compound_map_t map;
    attributes_t deps;
    attributes_t compound;
    for (_att_t::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
        attributes_t::value_type nonCompound(it->key(), it->isForbidden());
        if (it->isCompound()) {
            map[it->key()].insert(it->compoundValue());
            compound.insert(nonCompound);
        }
        deps.insert(nonCompound);
    }
    att_list_t multiplexed;
    multiplexed.push_back(deps);
    for (attributes_t::const_iterator it = compound.begin(); it != compound.end(); ++it) {
        const string_t& value = it->value();
        compound_map_t::const_iterator m_it = map.find(value);
        if (m_it != map.end()) {
            const compound_attributes_t& comps = m_it->second;
            multiplexed = multiplexAttributes(((it->isForbidden()) ? "!" + it->key() : it->key()), comps, multiplexed);
        }
    }
    for (att_list_t::const_iterator it = multiplexed.begin(); it != multiplexed.end(); ++it) {
        WW::TestStep step;
        step.short_desc(short_desc);
        step.dependencies(*it);
        step.changes(changes);
        step.required(isRequired);
        step.cost(cost);
        step.description(description);
        step.script(script);
        add(step);
    }
}

///
///
///

WW::Steps::Steps()
: m_pimpl(new Impl)
{
}

WW::Steps::Steps(std::istream& str)
: m_pimpl(new Impl)
{
    m_pimpl->add(str);
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

void
WW::Steps::addStep(const std::string& step)
{
    std::istringstream ist(step);
    m_pimpl->add(ist);
}

void
WW::Steps::addStep(std::istream& str)
{
    m_pimpl->add(str);
}

WW::StepList
WW::Steps::calculate() const
{
    StepList chain = m_pimpl->calculate();
    return chain;
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
    stepstore_t& steps = m_pimpl->allSteps();
    expandCompoundAttributes(steps);

    StepList result;
    for (stepstore_t::const_iterator it = steps.begin(); it != steps.end(); ++it)
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

void
WW::Steps::setShowProgress(bool showProgress)
{
    m_pimpl->setShowProgress(showProgress);
}

size_t
WW::Steps::size() const
{
    return m_pimpl->allSteps().size();
}

const WW::TestStep&
WW::Steps::front() const
{
    return m_pimpl->allSteps().front();
}

WW::TestStep&
WW::Steps::front()
{
    return m_pimpl->allSteps().front();
}
