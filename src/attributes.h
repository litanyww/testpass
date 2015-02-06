// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_ATTRIBUTES_HEADER
#define INCLUDE_WW_ATTRIBUTES_HEADER

#include "attribute.h"
#include "utils.h"

#include <set>

namespace WW
{
    template <class _T>
        class Attributes
        {
        public:
            typedef Attribute<_T> value_type;
            typedef typename value_type::size_type size_type;
            typedef std::set<value_type, typename value_type::setCompare> container_t;
            typedef value_type& reference;
            typedef value_type* pointer;
            typedef typename container_t::const_iterator const_iterator;
            typedef typename container_t::iterator iterator;
            typedef typename container_t::reverse_iterator reverse_iterator;
            typedef typename container_t::const_reverse_iterator const_reverse_iterator;

        private:
            container_t m_contents;

        public: // construction
            ~Attributes() {}
            Attributes() : m_contents() {}
            Attributes(const _T& element);
            Attributes(const value_type& element) : m_contents() { m_contents.insert(element); }
            Attributes(const Attributes& copy) : m_contents(copy.m_contents) {}
            Attributes& operator=(const Attributes& copy) { m_contents = copy.m_contents; return *this; }
#if __cplusplus >= 201103L
            Attributes(Attributes&& copy) : m_contents(copy.m_contents) {}
#endif

        public: // iterators
            const_iterator begin() const { return m_contents.begin(); }
            iterator begin() { return m_contents.begin(); }
            const_iterator end() const { return m_contents.end(); }
            iterator end() { return m_contents.end(); }
            const_reverse_iterator rbegin() const { return m_contents.rbegin(); }
            reverse_iterator rbegin() { return m_contents.rbegin(); }
            const_reverse_iterator rend() const { return m_contents.rend(); }
            reverse_iterator rend() { return m_contents.rend(); }
#if __cplusplus >= 201103L
            const_iterator cbegin() const { return m_contents.cbegin(); }
            const_iterator cend() const { return m_contents.cend(); }
            const_reverse_iterator crbegin() const { return m_contents.crbegin(); }
            const_reverse_iterator crend() const { return m_contents.crend(); }
#endif

        public:
            size_type size() const { return m_contents.size(); }
            std::pair<iterator,bool> require(const _T& val) { return insert(value_type(val)); }
            std::pair<iterator,bool> forbid(const _T& val) { return insert(value_type(val, true)); }
            std::pair<iterator,bool> insert(const value_type& val) {
                iterator it = m_contents.find(val);
                if (it != m_contents.end()) {
                   if (it->isForbidden() != val.isForbidden() || *it != val) {
                       iterator hint = it;
                       ++hint;
                       m_contents.erase(it);
                       m_contents.insert(hint, val);
                       it = m_contents.find(val); // recalculate iterator
                   }
                   return std::pair<iterator,bool>(it, true);
                }
                return m_contents.insert(val);
            }

            iterator insert(iterator position, const value_type& val) {
                iterator it = m_contents.insert(position, val);
                if (it->isForbidden() == val.isForbidden())
                {
                    return it; // all good
                }
                m_contents.erase(it);
                return m_contents.insert(position, val);
            }
            template <class InputIterator>
                void insert(InputIterator first, InputIterator last) {
                    while (first != last) {
                        insert(*first); // re-use our `input(value_type&)` method
                        ++first;
                    }
                }
            size_type count(const value_type& val) const { return m_contents.count(); }
            void erase(iterator position) { m_contents.erase(position); }
            size_type erase(const value_type& val) { return m_contents.erase(val); }
            size_type erase(const _T& val) { return m_contents.erase(value_type(val)); }
            void erase(iterator first, iterator last) { m_contents.erase(first, last); }
            template <class _Type>
                void applyChanges(_Type collection) {
                    const_iterator end = collection.end();
                    for (const_iterator it = collection.begin(); it != end; ++it) {
                        if (it->isForbidden()) {
                            m_contents.erase(it->value());
                        }
                        else
                        {
                            insert(*it);
                        }
                    }
                }
            iterator find(const value_type& val) const { iterator result = m_contents.find(val); return (*result == val) ? result : m_contents.end();}
            iterator find(const _T& val) const { iterator result = m_contents.find(value_type(val)); return (result->value() == val) ? result : m_contents.end();}
            void clear() { return m_contents.clear(); }
            bool operator!=(const Attributes& rhs) const { return !(*this == rhs); }
            bool operator==(const Attributes& rhs) const {
                bool result = false;
                if (size() == rhs.size()) {
                    result = true;
                    const_iterator l_it = begin();
                    const_iterator r_it = rhs.begin();
                    const_iterator l_end = end();
                    for (; l_it != l_end; ++l_it, ++r_it) {
                        if (*l_it != *r_it) {

                            result = false;
                            break;
                        }
                    }
                }
                return result;
            }
            bool containsAll(const Attributes& needle) const {
                bool result = true;
                const_iterator needleEnd = needle.end();
                const_iterator notfound = m_contents.end();
                for (const_iterator it = needle.begin(); it != needleEnd; ++it)
                {
                    const_iterator match = m_contents.find(*it);
                    if (match == notfound || match->value() != it->value())
                    {
                        if (!it->isForbidden())
                        {
                            result = false;
                            break;
                        }
                    }
                    else
                    {
                        if (
                                (it->isForbidden() && !match->isForbidden()) ||
                                (!it->isForbidden() && match->isForbidden()))
                        {
                            result = false;
                            break;
                        }
                    }
                }
                return result;
            }

            bool containsAny(const Attributes& needle) const {
                // haystack = ['one',  'two', 'three'], needle =  'two' *MATCH*
                // haystack = ['one',  'two', 'three'], needle = '!two' *FAIL*
                // haystack = ['one', '!two', 'three'], needle =  'two' *FAIL*
                // haystack = ['one', '!two', 'three'], needle = '!two' *MATCH*
                bool result = false;
                const_iterator needleEnd = needle.end();
                const_iterator notfound = m_contents.end();
                for (const_iterator it = needle.begin(); it != needleEnd; ++it)
                {
                    const_iterator match = m_contents.find(*it);
                    if (match != notfound && match->value() == it->value())
                    {
                        if (it->isForbidden() != match->isForbidden())
                        {
                            result = false;
                            break;
                        }
                        result = true;
                    }
                }
                return result;
            }

            static void find_changes(const Attributes& state, const Attributes& target, Attributes& out_required, Attributes& out_discard)
            {
                out_required.clear();
                out_discard.clear();

                const_iterator end = target.end();
                const_iterator notfound = state.end();

                for (const_iterator t_it = target.begin(); t_it != end; ++t_it)
                {
                    bool forbidden = t_it->isForbidden();
                    const_iterator match = state.m_contents.find(*t_it);
                    if (match == notfound) {
                        if (!forbidden) {
                            out_required.insert(*t_it);
                        }
                    }
                    else if (match->value() != t_it->value()) { // compound
                        if (!forbidden) {
                            if (!match->isForbidden()) {
                                out_discard.insert(value_type(match->value(), true));
                                out_required.insert(*t_it);
                            }
                        }
                        else if (t_it->value().find('=') == value_type::value_type::npos) {
                            out_discard.insert(value_type(match->value(), true));
                        }
                    }
                    else if (forbidden) // match, but now forbidden
                    {
                        out_discard.forbid(t_it->value());
                    }
                }
            }

            Attributes differences(const Attributes& attributes) const
            {
                Attributes result;
                return differences(attributes, result);
            }
            Attributes& differences(const Attributes& state, Attributes& out_result) const
            {
                out_result.clear();
                const_iterator end = state.end();
                const_iterator notfound = m_contents.end();
                for (const_iterator it = state.begin(); it != end; ++it)
                {
                    bool forbidden = it->isForbidden();
                    const_iterator match = m_contents.find(*it);
                    if (match == notfound) {
                        if (!forbidden) {
                            out_result.insert(*it);
                        }
                    }
                    else if (match->value() != it->value()) { // compound
                        if (!forbidden) {
                            if (!match->isForbidden()) {
                                out_result.insert(value_type(match->value(), true));
                                out_result.insert(*it);
                            }
                        }
                        else if (it->value().find('=') == value_type::value_type::npos) {
                            out_result.insert(value_type(match->value(), true));
                        }
                    }
                    else if (forbidden)
                    {
                        out_result.forbid(it->value());
                    }
                }
                return out_result;
            }
        };

    template <typename _T>
        Attributes<_T>::Attributes(const _T& element) : m_contents()
        {
            m_contents.clear();
            std::string::size_type start = 0;
            std::string::size_type pos = element.find(',');
            while (pos != std::string::npos)
            {
                if (element[start] == '!')
                {
                    forbid(strip(element.substr(start + 1, pos - start - 1)));
                }
                else
                {
                    require(strip(element.substr(start, pos - start)));
                }
                start = pos + 1;
                pos = element.find(',', start);
            }
            if (element[start] == '!')
            {
                forbid(strip(element.substr(start + 1)));
            }
            else
            {
                require(strip(element.substr(start)));
            }
        }

    template <class Stream, class _T>
        Stream&
        operator<<(Stream& ost, const Attributes<_T>& ob)
        {
            ost << "[";
            bool comma = false;
            for (typename Attributes<_T>::const_iterator it = ob.begin(); it != ob.end(); ++it) {
                if (comma)
                {
                    ost << ",";
                }
                else
                {
                    comma = true;
                }
                ost << *it;
            }
            ost << "]";
            return ost;
        }
}
#endif // INCLUDE_WW_ATTRIBUTES_HEADER
