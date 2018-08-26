/*!
 * \file
 * \brief     Cercall program information
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

#include "program.h"

#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

#include <iostream>

static Program* myInstance = nullptr;   //Shall never be freed.

Program::Program()
{
#ifdef __linux__
    char progPath[1024];
    ssize_t nameLen = readlink ( "/proc/self/exe", progPath, sizeof (progPath) );
    progPath[nameLen] = 0;
#elif _WIN32
    char progPath[_MAX_PATH+1];
    GetModuleFileName(NULL, progPath, _MAX_PATH);
#endif
    myPath = std::string(progPath);
    myName = myPath.substr(myPath.find_last_of('/') + 1);
}

Program& Program::instance()
{
    if (myInstance == nullptr) {
        myInstance = new Program();
    }
    return *myInstance;
}

const char * Program::get_name() const
{
    return myName.c_str();
}

const char* Program::get_path() const
{
    return myPath.c_str();
}
