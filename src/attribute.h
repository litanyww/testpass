// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_ATTRIBUTE_HEADER
#define INCLUDE_WW_ATTRIBUTE_HEADER

#include <ostream>

namespace {
    template <typename _T>
        _T noEquals(const _T& val) {
            typename _T::size_type e = val.find_first_of("=");
            if (e == _T::npos) {
                return val;
            }
            return val.substr(0, e);
        }
}

namespace WW
{
    template <class _T>
        class Attribute
        {
        public:
            typedef _T value_type;
            typedef typename _T::size_type size_type;
            
            /// use our own set comparison functor to disallow the same value to be forbidden and not-forbidden
            struct setCompare {
                bool operator() (const Attribute& lhs, const Attribute& rhs) const {
                    return noEquals(lhs.value()) < noEquals(rhs.value());
                }
            };

        public:
            ~Attribute() {}
            Attribute() : m_value(), m_forbidden(false) {}
            Attribute(const _T& value) : m_value(value), m_forbidden(false) {}
            Attribute(const _T& value, bool forbidden) : m_value(value), m_forbidden(forbidden) {}
            Attribute(const Attribute& copy) : m_value(copy.m_value), m_forbidden(copy.m_forbidden) {}
            Attribute& operator=(const Attribute& copy) { m_value = copy.m_value; m_forbidden == copy.m_forbidden; }
#if __cplusplus >= 201103L
            Attribute(Attribute&& copy) : m_value(copy.m_value), m_forbidden(copy.m_forbidden) {}
#endif

        public:
            bool operator==(const Attribute& rhs) const { return m_value == rhs.m_value && m_forbidden == rhs.m_forbidden; }
            bool operator!=(const Attribute& rhs) const { return !(*this == rhs); }

        public:
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

#endif // INCLUDE_WW_ATTRIBUTE_HEADER
