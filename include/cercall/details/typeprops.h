/*!
 * \file
 * \brief     Cercall library details - type properties
 *
 *  Copyright (c) 2018, Arthur Wisz
 *  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CERCALL_DETAILS_TYPEPROPS_H
#define CERCALL_DETAILS_TYPEPROPS_H

#include <type_traits>
#include "cercall/details/typeutil.h"

/**
 * @def O_REGISTER_FORWARD_DECLARED_TYPE(typeName)
 * @brief A macro to decorate a type with additional compile-time properties.
 * The class has to be forward-declared in its namespace before using this macro.
 * @param typeName a fully qualified type name (with namespace prefix)
 * After a type is registered, the following type properties are available at compile time:
 * - TypeProperties<namespace::Type>::name - the type name ("namespace::Type" here)
 * - TypeProperties<namespace::Type>::hashCode - a hash code of the type name.
 */
#define O_REGISTER_FORWARD_DECLARED_TYPE(typeName)  \
namespace cercall {    \
namespace details {    \
template<> struct TypeProperties<typeName>  {    \
    static constexpr auto name = #typeName; \
};}}


#define O_REGISTER_FORWARD_DECLARED_DERIVED_TYPE(typeName, baseTypeName)  \
namespace cercall {    \
namespace details {    \
template<> struct TypeProperties<typeName>  {                               \
    static constexpr auto name = #typeName;                                 \
    using BaseTypeName = baseTypeName;                                      \
};}}

/**
 * @def O_REGISTER_TYPE(typeName)
 * @brief A macro to decorate a type with additional compile-time properties.
 * After the type is registered, the following type properties are available at compile time:
 * - TypeProperties<Type>::name - the type name ("Type" here)
 * - TypeProperties<Type>::hash_code - a hash code of the type name.
 */
#define O_REGISTER_TYPE(typeName)  \
class typeName;     \
O_REGISTER_FORWARD_DECLARED_TYPE(typeName)

#define O_REGISTER_DERIVED_TYPE(typeName, baseTypeName)  \
class typeName;     \
O_REGISTER_FORWARD_DECLARED_DERIVED_TYPE(typeName, baseTypeName)

namespace cercall {
namespace details {

/** @private */
template<class T>
struct TypeProperties {
};

}   //details
}   //cercall


#endif // CERCALL_DETAILS_TYPEPROPS_H
