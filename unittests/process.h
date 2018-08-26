/*!
 * \file
 * \brief     Cercall tests - Process class interface
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

#ifndef CERCALL_PROCESS_H
#define CERCALL_PROCESS_H

#include <utility>

#if defined(_WIN32)
#include <string>
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace cercall {
namespace test {

class Process
{
private:
    struct System
    {
#if defined(_WIN32)
        typedef DWORD ExitStatusType;
        typedef DWORD ProcessId;

        enum { AccessViolation = STATUS_ACCESS_VIOLATION };
        enum { ProcessTerminated = 257 };
#elif defined(__linux__)
        typedef int ExitStatusType;
        typedef pid_t ProcessId;

        enum { AccessViolation = 11 };
        enum { ProcessTerminated = 9 };
#endif
    };

public:
    typedef std::pair<bool, System::ExitStatusType> ExitStatusType;
    typedef System::ProcessId ProcessId;

    enum ExceptionType
    {
        AccessViolation = System::AccessViolation,
        ProcessTerminated = System::ProcessTerminated
    };

    Process();
    ~Process();

    /**
     * \brief Create a new process.
     *
     * \param programPath Full path to a program file to be executed.
     * \param arguments Program arguments, separated by spaces.
     * \warning This method doesn't exists on unicode platforms. (Windows in unicode mode)
     * \throw std::runtime_error Thrown if creating process failed.
     */
#ifndef UNICODE
    void create(char const* programPath, char const* arguments);
#endif // #ifndef UNICODE

    /**
     * \brief Create a new process.
     *
     * \param programPath Full path to a program file to be executed.
     * \param arguments Program arguments, separated by spaces.
     * \warning This method doesn't exists on ascii platforms. (Linux and Windows in mbs mode)
     * \throw std::runtime_error Thrown if creating process failed.
     */
#ifdef UNICODE
    void create(wchar_t const* programPath, wchar_t const* arguments);
#endif // #ifdef UNICODE

    /**
     * \return true if the process has been created and is running.
     */
    bool is_running() const;

    /**
     * \brief Block and wait for the process until it terminates.
     * \return the exit status of the finished process.
     */
    ExitStatusType wait();

    /**
     * \brief Terminate the process.
     * This is a hard-kill function. It sends SIGKILL to the process on linux, and
     * calls TerminateProcess on Windows.
     */
    void kill();

    /**
     * \brief Request an orderly exit of the process.
     * On Linux it sends SIGTERM to the managed process.
     */
    void shutdown();

    /**
     * \brief Terminate the process with the given pid.
     */
    static void kill(ProcessId pid);

    /**
     * \return the process id of the current process.
     */
    static ProcessId get_pid();

private:

#if defined(_WIN32)
    static void throw_error(std::string const& errorMessage);
    static void get_error_message(DWORD error, std::string& errorMessage);
#endif

#if defined(_WIN32)
    HANDLE m_procHandle;
#elif defined(__linux__)
    pid_t myPid;
#endif
};

}   //namespace test
}   //namespace cercall

#endif // #ifndef CERCALL_PROCESS_H
