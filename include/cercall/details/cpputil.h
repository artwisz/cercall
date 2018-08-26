/*!
 * \file
 * \brief     Cercall preprocessor utilities
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


#ifndef CERCALL_CPPUTIL_H
#define CERCALL_CPPUTIL_H

#include <memory>

/* This counts the number of macro args. */
#define CERCALL_NARGS_SEQ(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define CERCALL_NARGS(...) CERCALL_NARGS_SEQ(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

/* This will let macros expand before concating them. */
#define CERCALL_PRIMITIVE_CAT(x, y) x ## y
#define CERCALL_CAT(x, y) CERCALL_PRIMITIVE_CAT(x, y)

#define CERCALL_STRINGIZE(dummy, y) #y

/* This will call a macro on each argument passed in. */
#define CERCALL_APPLY(macro, arg, ...) CERCALL_CAT(CERCALL_APPLY_, CERCALL_NARGS(__VA_ARGS__))(macro, arg, __VA_ARGS__)
#define CERCALL_APPLY_1(m, arg, x1) m(arg, x1)
#define CERCALL_APPLY_2(m, arg, x1, x2) m(arg, x1), m(arg, x2)
#define CERCALL_APPLY_3(m, arg, x1, x2, x3) m(arg, x1), m(arg, x2), m(arg, x3)
#define CERCALL_APPLY_4(m, arg, x1, x2, x3, x4) m(arg, x1), m(arg, x2), m(arg, x3), m(arg, x4)
#define CERCALL_APPLY_5(m, arg, x1, x2, x3, x4, x5) m(arg, x1), m(arg, x2), m(arg, x3), m(arg, x4), m(arg, x5)
#define CERCALL_APPLY_6(m, arg, x1, x2, x3, x4, x5, x6) m(arg, x1), m(arg, x2), m(arg, x3), m(arg, x4), m(arg, x5), m(arg, x6)
#define CERCALL_APPLY_7(m, arg, x1, x2, x3, x4, x5, x6, x7) m(arg, x1), m(arg, x2), m(arg, x3), m(arg, x4), m(arg, x5), m(arg, x6), m(arg, x7)
#define CERCALL_APPLY_8(m, arg, x1, x2, x3, x4, x5, x6, x7, x8) m(arg, x1), m(arg, x2), m(arg, x3), m(arg, x4), m(arg, x5), m(arg, x6), m(arg, x7), m(arg, x8)

/* A macro applied to make a pointer to member function. */
#define CERCALL_MAKE_MEMFUN(i, f)  (&i::f)

namespace cercall {

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

}

#endif // CERCALL_CPPUTIL_H
