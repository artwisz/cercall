/*!
 * \file
 * \brief     Cercall logging
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

#ifndef CERCALL_LOG_H
#define CERCALL_LOG_H

#include <cstdlib>

namespace cercall_default_log {

enum LogLevel
{
    off,
    fatal,
    error,
    debug,
    trace
};

template<LogLevel ll, typename... Args>
void log(LogLevel, const char*, const char*, Args...)
{
}

inline void assert_failed()
{
    abort();
}

}   //namespace cercall_default_log

/* A log() function in the namespace cercall_user_log will hide the one from cercall_default_log. */
namespace cercall_user_log {
    using namespace cercall_default_log;
}

/**
 * This preprocessor token is expanded on every call of the log<>() function and passed to it as the
 * first parameter. Its purpose is to identify the function in the log. The library user can define it
 * to whatever the compiler offers, like __PRETTY_FUNCTION__, or __FUNCTION__.
 * The default is what the standard has to offer.
 */
#ifndef O_LOG_TOKEN
#define O_LOG_TOKEN  __func__
#endif

namespace cercall {

enum LogLevel
{
    off   = cercall_user_log::off,
    fatal = cercall_user_log::fatal,
    error = cercall_user_log::error,
    debug = cercall_user_log::debug,
    trace = cercall_user_log::trace
};

template<LogLevel ll, typename... Args>
void log(const char* logToken, const char* format, Args... args)
{
    constexpr cercall_user_log::LogLevel logLevel = static_cast<cercall_user_log::LogLevel>(ll);
    cercall_user_log::log<logLevel>(logLevel, logToken, format, args...);
}

inline void log_assert(bool cond, const char* file, int line, const char* condStr)
{
#ifndef NDEBUG
    if ( !cond) {
        constexpr cercall_user_log::LogLevel logLevel = cercall_user_log::fatal;
        cercall_user_log::log<logLevel>(logLevel, "", "assertion '%s' failed at %s:%d", condStr, file, line);
        cercall_user_log::assert_failed();
    }
#else
    (void)cond;
    (void)file;
    (void)line;
    (void)condStr;
#endif
}

/* A cercall replacement of 'assert' */
#define o_assert(cond) (static_cast<bool>(cond) ? (void)0 \
                             : log_assert(static_cast<bool>(cond), __FILE__, __LINE__, #cond))

} //namespace cercall

#endif //CERCALL_LOG_H
