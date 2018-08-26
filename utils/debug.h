/*!
 * \file
 * \brief     Cercall debug log functions for tests and examples
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

#ifndef CERCALL_DEBUG_H
#define CERCALL_DEBUG_H

#include <type_traits>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <cstring>
#include <regex>
#include "program.h"

namespace cercall_user_log {

/* User log levels are configurable, but must be unique */
enum LogLevel
{
    off = 0,
    fatal = 1,
    error = 2,
    debug = 3,
    trace = 5
};

#ifndef ENABLED_LOG_LEVEL
#define ENABLED_LOG_LEVEL  debug        /* This can also be set in the preprocessor command line */
#endif

#define O_LOG_TOKEN  __PRETTY_FUNCTION__

#pragma GCC diagnostic ignored "-Wformat-security"

inline
const char* levelToStr(LogLevel l)
{
    switch (l) {
        case fatal: return "F";
        case error: return "E";
        case debug: return "D";
        case trace: return "T";
        default: return "?";
    }
}

template<LogLevel StaticLevel, typename... Args>
void log(LogLevel ll, const char* logToken, const char* format, Args... args)
{
    if /* constexpr */ (StaticLevel <= ENABLED_LOG_LEVEL) {
        fprintf(stdout, "%s [%s] ", Program::instance().get_name(), levelToStr(ll));
        if (*logToken != 0) {
            std::string tok = std::regex_replace(logToken, std::regex("([^[]*) \\[with .*\\](.*)"), "$1$2");
            do {
                tok = std::regex_replace(tok, std::regex("([^(]*)\\([^)]*\\)(.*)"), "$1$2");
            } while (tok.find_first_of('(') != tok.npos);

            do {
                tok = std::regex_replace(tok, std::regex("([^<]*)<[^>]*>(.*)"), "$1$2");
            } while (tok.find_first_of('<') != tok.npos);

            size_t posLastSpace = tok.find_last_of(' ');
            if (posLastSpace != tok.npos) {
                tok = tok.substr(posLastSpace + 1);
            }
            fprintf(stdout, "%s", tok.c_str());
            //fprintf(stdout, "%s", logToken);
        }
        if (*format != 0) {
            fprintf(stdout, ": ");
            fprintf(stdout, format, args...);
        }
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

}   //namespace cercall_user_log

#endif //CERCALL_DEBUG_H
