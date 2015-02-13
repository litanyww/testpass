// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_ATTRIBUTE_HEADER
#define INCLUDE_WW_ATTRIBUTE_HEADER

#include <ostream>
#include <iostream>

namespace WW
{
    template <class _T>
        class Attribute
        {
        public:
            typedef _T value_type;
            typedef typename _T::size_type size_type;
            
            /// use our own set comparison functor to disallow the same value to be forbidden and not-forbidden.
            struct setCompare {
                bool operator() (const Attribute& lhs, const Attribute& rhs) const {
                    typename _T::const_pointer lc = lhs.m_value.c_str();
                    typename _T::const_pointer rc = rhs.m_value.c_str();
                    typename _T::size_type offset = 0;
                    typename _T::size_type lhs_size = lhs.m_value.size();
                    typename _T::size_type rhs_size = rhs.m_value.size();
                    typename _T::size_type len = (lhs_size > rhs_size) ? rhs_size : lhs_size;

                    for (offset = 0; offset < len; ++offset) {
                        const typename _T::value_type lhs_c = lc[offset];
                        const typename _T::value_type rhs_c = rc[offset];
                        if (lhs_c == rhs_c) {
                           if (lhs_c != '=') {
                               continue;
                           }
                           // both compound, both have same prefix
                           return false; // treat as identical
                        }
                        return lhs_c < rhs_c;
                    }
                    if (lhs_size == rhs_size) { // exactly the same, neither has '='
                        return false;
                    }
                    if (((len == lhs_size) ? rc[len] : lc[len]) == '=') { // compound prefix of one is same as entire other
                        return false;
                    }
                    return (len == lhs_size) ? true : false;
                }
            };

        public:
            ~Attribute() {}
            Attribute() : m_value(), m_forbidden(false) {}
            Attribute(const _T& value) : m_value(value), m_forbidden(false) {}
            Attribute(const _T& value, bool forbidden) : m_value(value), m_forbidden(forbidden) {}
            Attribute(const Attribute& copy) : m_value(copy.m_value), m_forbidden(copy.m_forbidden) {}
            Attribute& operator=(const Attribute& copy) { m_value = copy.m_value; m_forbidden = copy.m_forbidden; return *this; }
#if __cplusplus >= 201103L
            Attribute(Attribute&& copy) : m_value(copy.m_value), m_forbidden(copy.m_forbidden) {}
#endif

        public:
            bool operator==(const Attribute& rhs) const { return m_value == rhs.m_value && m_forbidden == rhs.m_forbidden; }
            bool operator!=(const Attribute& rhs) const { return !(*this == rhs); }

        public:
            _T key() const { typename _T::size_type pos = m_value.find("="); if (pos != _T::npos) { return m_value.substr(0, pos); } return m_value; }
            const _T& value() const { return m_value; }
            bool isForbidden() const { return m_forbidden; }

        private:
            _T m_value;
            bool m_forbidden;
        };

    template <class _T, class _U>
        bool operator==(const _T& lhs, const Attribute<_U>& rhs) { return _U(lhs) == rhs.value(); }
    template <class _T, class _U>
        bool operator!=(const _T& lhs, const Attribute<_U>& rhs) { return _U(lhs) != rhs.value(); }
    template <class _T, class _U>
        bool operator==(const Attribute<_T>& lhs, const _U& rhs) { return lhs.value() == _T(rhs); }
    template <class _T, class _U>
        bool operator!=(const Attribute<_T>& lhs, const _U& rhs) { return lhs.value() != _T(rhs); }

    template <class Stream, class _T>
        Stream& operator<<(Stream& str, const Attribute<_T>& attr)
        {
            if (attr.isForbidden()) {
                str << "!";
            }
            str << attr.value();
            return str;
        }

    template <class _T>
        void PrintTo(const Attribute<_T>& attr, ::std::ostream* str) {
            *str << attr;
        }
}

namespace std
{
    template <typename _T>
    struct less<WW::Attribute<_T> > : public WW::Attribute<_T>::setCompare {};
}

#endif // INCLUDE_WW_ATTRIBUTE_HEADER
