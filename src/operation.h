// Copyright 2015 Sophos Limited. All rights reserved.
//
// Sophos is a registered trademark of Sophos Limited and Sophos Group.
//

#ifndef INCLUDE_WW_OPERATION_HEADER
#define INCLUDE_WW_OPERATION_HEADER

#include "attributes.h"

namespace WW
{
    template <class _T>
        class Operation
        {
        public:
            typedef Attributes<_T> value_type;
        public:
            ~Operation() {}
            Operation() : m_dependencies(), m_forbidden(), m_added(), m_removed() {}
#if __cplusplus >= 201103L
            Operation(const Operation& copy) = default;
            Operation& operator=(const Operation& copy) = default;
            Operation(Operation&& copy) = default;
#endif
        public:
            void setDependencies(const value_type& attributes) { m_dependencies = attributes; }
            void setForbidden(const value_type& attributes) { m_forbidden = attributes; }
            void setAdded(const value_type& attributes) { m_added = attributes; }
            void setRemoved(const value_type& attributes) { m_removed = attributes; }
            /** Modify a set of attributes according to this operation */
            void modify(value_type& attributes) const {
                attributes.remove(m_removed);
                attributes.add(m_added);
            }
            /** Create a new set of attributes, modified by this operation */
            value_type apply(const value_type& attributes) const {
                value_type result = attributes;
                modify(result);
                return result;
            }
            bool isValid(const value_type& attributes) const { return attributes.containsAll(m_dependencies) && !attributes.containsAny(m_forbidden); }

        public:

        private:
            value_type m_dependencies;
            value_type m_forbidden;
            value_type m_added;
            value_type m_removed;
        };
}

#endif // INCLUDE_WW_OPERATION_HEADER
