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


#ifndef CERCALL_PROGRAM_H
#define CERCALL_PROGRAM_H

#include <string>

class Program
{
public:
    const char* get_name() const;
    const char* get_path() const;

    static Program& instance();

private:
    Program();

    std::string myName;
    std::string myPath;
};

#endif // CERCALL_PROGRAM_H
