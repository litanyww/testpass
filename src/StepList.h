// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_STEPLIST_HEADER
#define INCLUDE_WW_STEPLIST_HEADER

#include "TestStep.h"

#include <algorithm>
#include <list>

namespace WW
{
    class StepList
    {
    public:
        typedef const TestStep value_type;
        typedef value_type::size_type size_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef std::list<const_pointer> container_t;

        class iterator : public std::iterator<
            std::bidirectional_iterator_tag,
            StepList::value_type,
            TestStep::size_type,
            StepList::pointer,
            StepList::reference>
            {
                typedef StepList::container_t::iterator it_t;
                StepList::container_t::iterator m_current;
            public:
                iterator() : m_current() {}
                iterator(const it_t it) : m_current(it) {}
            public:
                it_t base() const { return m_current; }
            public:
                reference operator*() const { return **m_current; }
                pointer operator->() const { return *m_current; }
                iterator& operator++()
                    {
                        ++m_current;
                        return *this;
                    }

                iterator
                    operator++(int)
                    {
                        return iterator(m_current++);
                    }
                iterator& operator--()
                    {
                        --m_current;
                        return *this;
                    }

                iterator
                    operator--(int)
                    {
                        return iterator(m_current--);
                    }
            };
        class const_iterator : public std::iterator<
            std::bidirectional_iterator_tag,
            const StepList::value_type,
            TestStep::size_type,
            const StepList::pointer,
            const StepList::reference>
            {
                typedef StepList::container_t::const_iterator it_t;
                it_t m_current;
            public:
                const_iterator() : m_current() {}
                const_iterator(it_t it) : m_current(it) {}
                const_iterator(StepList::iterator it) : m_current(it.base()) {}
            public:
                it_t base() const { return m_current; }
            public:
                reference operator*() { return **m_current; }
                pointer operator->() { return *m_current; }
                const_iterator& operator++()
                    {
                        ++m_current;
                        return *this;
                    }

                const_iterator
                    operator++(int)
                    {
                        return const_iterator(m_current++);
                    }
                const_iterator& operator--()
                    {
                        --m_current;
                        return *this;
                    }

                const_iterator
                    operator--(int)
                    {
                        return const_iterator(m_current--);
                    }
            };

        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    public:
        ~StepList() {}
        StepList() : m_contents() {}
        StepList(const StepList& copy) : m_contents(copy.m_contents) {}
        StepList(const StepList::container_t& copy) : m_contents(copy) {}
        StepList(const_iterator it1, const_iterator it2) : m_contents(it1.base(), it2.base()) {}

    private:
        container_t m_contents;

    public: // iterators
        const_iterator begin() const { return const_iterator(m_contents.begin()); }
        iterator begin() { return iterator(m_contents.begin()); }
        const_iterator end() const { return const_iterator(m_contents.end()); }
        iterator end() { return iterator(m_contents.end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(m_contents.rbegin()); }
        reverse_iterator rbegin() { return reverse_iterator(m_contents.rbegin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(m_contents.rend()); }
        reverse_iterator rend() { return reverse_iterator(m_contents.rend()); }
#if __cplusplus >= 201103L
        const_iterator cbegin() const { return const_iterator(m_contents.cbegin()); }
        const_iterator cend() const { return const_iterator(m_contents.cend()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(m_contents.crbegin()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(m_contents.crend()); }
#endif
    public:
        void clear() { return m_contents.clear(); }
        bool empty() { return m_contents.empty(); }
#if __cplusplus >= 201103L
        void splice(const_iterator __position, StepList&& __x) noexcept { m_contents.splice(__position.base(), __x.m_contents); }
        void splice(const_iterator __position, StepList& __x) noexcept { m_contents.splice(__position.base(), __x.m_contents); }
#else
        void splice(iterator __position, StepList& __x) { m_contents.splice(__position.base(), __x.m_contents); }
#endif

        size_type size() const { return m_contents.size(); }
        void erase(iterator position) { m_contents.erase(position.base()); }
        void erase(iterator first, iterator last) { m_contents.erase(first.base(), last.base()); }
        void push_back(pointer val) { m_contents.push_back(val); }
#if __cplusplus >= 201103L
        iterator insert(const_iterator pos, const value_type& val) { return iterator(m_contents.insert(pos.base(), &val)); }
#else
        iterator insert(iterator pos, const value_type& val) { return iterator(m_contents.insert(pos.base(), &val)); }
#endif
        const_iterator find(const_reference val) const { return std::find(begin(), end(), val); }
        iterator find(reference val) { return std::find(begin(), end(), val); }
        void append(const StepList& steps) {
            for (container_t::const_iterator it = steps.m_contents.begin(); it != steps.m_contents.end(); ++it) {
                m_contents.push_back(*it);
            }
        }
    };

    inline bool operator==(const StepList::iterator lhs, const StepList::iterator rhs) {
        return lhs.base() == rhs.base();
    }

    inline bool operator!=(const StepList::iterator lhs, const StepList::iterator rhs) {
        return lhs.base() != rhs.base();
    }

    inline bool operator==(const StepList::const_iterator lhs, const StepList::const_iterator rhs) {
        return lhs.base() == rhs.base();
    }

    inline bool operator!=(const StepList::const_iterator lhs, const StepList::const_iterator rhs) {
        return lhs.base() != rhs.base();
    }

    inline StepList operator+(const StepList& lhs, const StepList& rhs) {
        StepList result(lhs);
        result.append(rhs);
        return result;
    }

    inline StepList operator+(const StepList& lhs, const TestStep& val) {
        StepList result = lhs;
        result.push_back(&val);
        return result;
    }

    inline StepList operator+(const TestStep& val, const StepList& lhs) { return lhs + val; }

    // For gtest
    inline void PrintTo(const StepList::iterator& it, ::std::ostream* str) {
        *str << "StepList::iterator";
    }
    inline void PrintTo(const StepList::const_iterator& it, ::std::ostream* str) {
        *str << "StepList::iterator";
    }

    template <typename Stream>
        Stream&
        operator<<(Stream& ost, const StepList& list) {
            bool comma = false;
            ost << "[";
            for (StepList::const_iterator it = list.begin(); it != list.end(); ++it)
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

inline WW::StepList::const_iterator
operator+(WW::StepList::const_iterator it, int count)
{
    if (count > 0) {
        while (--count > 0) {
            ++it;
        }
    }
    else if (count < 0) {
        while (++count < 0) {
            --it;
        }
    }
    return it;
}

inline WW::StepList::const_iterator
operator-(WW::StepList::const_iterator it, int count)
{
    if (count > 0) {
        while (--count > 0) {
            --it;
        }
    }
    else if (count < 0) {
        while (++count < 0) {
            ++it;
        }
    }
    return it;
}

template <class Iter>
struct iterator_traits {
    typedef typename Iter::value_type value_type;
    typedef typename Iter::difference_type difference_type;
    typedef typename Iter::iterator_category iterator_category;
    typedef typename Iter::pointer pointer;
    typedef typename Iter::reference reference;
};

#endif // INCLUDE_WW_STEPLIST_HEADER
