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
            Operation() : m_dependencies(), m_changes() {}
#if __cplusplus >= 201103L
            Operation(const Operation& copy) = default;
            Operation& operator=(const Operation& copy) = default;
            Operation(Operation&& copy) = default;
#endif
        public:
            void setDependencies(const value_type& attributes) { m_dependencies = attributes; }
            void setChanges(const value_type& attributes) { m_changes = attributes; }
            /** Modify a set of attributes according to this operation */
            void modify(value_type& attributes) const {
                attributes.applyChanges(m_changes);
            }
            /** Create a new set of attributes, modified by this operation */
            value_type apply(const value_type& attributes) const {
                value_type result = attributes;
                modify(result);
                return result;
            }
            bool isValid(const value_type& attributes) const { return attributes.containsAll(m_dependencies); }
            bool hasChanges() const { return m_changes.size() != 0; }
            value_type getDifferences(const value_type& state) const {
                value_type result;
                return state.differences(m_dependencies, result);
            }
            value_type& getDifferences(const value_type& state, value_type& out_result) const {
                return state.differences(m_dependencies);
            }

        private:
            value_type m_dependencies;
            value_type m_changes;
        };
}

#endif // INCLUDE_WW_OPERATION_HEADER
