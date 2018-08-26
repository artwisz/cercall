/*!
 * \file
 * \brief     Cercall library details - some metaprogramming utilities
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

#ifndef CERCALL_TYPEUTIL_H
#define CERCALL_TYPEUTIL_H

namespace cercall {
namespace details {

template<typename ...>
struct type_tuple {};

/** Get last type of a parameter pack. */
template<typename Head, typename... Tail>
struct last_type_of {
   typedef typename last_type_of<Tail...>::type type;
};

template<typename Tail>
struct last_type_of<Tail> {
  typedef Tail type;
};

template<typename ...Ts>
struct remove_last_type_priv;

template<typename ...L, typename M, typename ...R>
struct remove_last_type_priv<type_tuple<L...>, M, R...>
{
    using type = typename remove_last_type_priv<type_tuple<L..., M>, R...>::type;
};

template<typename ...L, typename M, typename R>
struct remove_last_type_priv<type_tuple<L...>, M, R>
{
    using type = type_tuple<L..., M>;
};

template<typename R>
struct remove_last_type_priv<type_tuple<>, R>
{
    using type = type_tuple<>;
};

/** Remove last type from a parameter pack. */
template<typename ...Ts>
struct remove_last_type_of
{
    using type = typename remove_last_type_priv<type_tuple<>, Ts...>::type;
};

}   //namespace details
}   //namespace cercall

#endif // CERCALL_TYPEUTIL_H
