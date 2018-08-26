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

#include "process.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <regex>
#include <iostream>

#ifdef __linux__

#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

using cercall::test::Process;

Process::Process() : myPid (-1)
{
}

Process::~Process()
{
}

static std::vector<std::string> split_args(const std::string& args) {
    static const std::regex re{"\\s+"};
    std::vector<std::string> container {
        std::sregex_token_iterator(args.begin(), args.end(), re, -1),
        std::sregex_token_iterator()
    };
    return container;
}

void Process::create(char const* programPath, char const* arguments)
{
    if ((0 == programPath) || (strlen(programPath) < 1))
        throw std::runtime_error("Process::create: path to executable file is empty.");
    pid_t result = fork();
    if (result < 0)
        throw std::runtime_error("fork: " + std::string(strerror(errno)));
    else if (0 == result)
    {
        std::vector<std::string> args { std::string(programPath) };
        if (arguments != nullptr) {
            std::vector<std::string> moreArgs = split_args(arguments);
            args.insert(args.end(), moreArgs.begin(), moreArgs.end());
        }

        char* argv[args.size() + 1];

        for(size_t i = 0; i < args.size(); ++i)
            argv[i] = const_cast<char*>(args[i].c_str());
        argv[args.size()] = 0;

        execv(programPath, argv);
        // If execution is still here it means execl method failed.
        exit(-1);
    }
    else
        myPid = result;
}

bool Process::is_running() const
{
    return ::kill(myPid, 0) == 0;
}

Process::ExitStatusType Process::wait()
{
    int status;

    if (waitpid(myPid, &status, 0) < 0)
        throw std::runtime_error("waitpid: " + std::string(strerror(errno)));

    if (WIFEXITED(status))
        return std::make_pair(true, WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        return std::make_pair(false, WTERMSIG(status));

    throw std::logic_error("Process::wait: unexpected exit status");
}

void Process::kill()
{
    int res = ::kill( myPid, SIGKILL);
    if (res < 0)
        throw std::runtime_error("Process::kill: " + std::string(strerror(errno)));
}

void Process::kill(ProcessId pid)
{
    int res = ::kill(pid, SIGKILL);
    if (res < 0)
        throw std::runtime_error("Process::kill(pid): " + std::string(strerror(errno)));
}

void Process::shutdown()
{
    int res = ::kill(myPid, SIGTERM);
    if (res < 0)
        throw std::runtime_error("Process::shutdown(pid): " + std::string(strerror(errno)));
}

Process::ProcessId Process::get_pid()
{
    return getpid();
}

#elif defined(_WIN32)

Process::Process() : myProcHandle(INVALID_HANDLE_VALUE)
{
}

Process::~Process()
{
    if (myProcHandle != INVALID_HANDLE_VALUE)
        CloseHandle(myProcHandle);
}

#ifdef UNICODE

void Process::create(wchar_t const* programPath, wchar_t const* arguments)
{
    if ((0 == programPath) || (wcslen(reinterpret_cast<const wchar_t*>(programPath)) < 1))
        throw std::runtime_error("Process::create: path to executable file is empty.");

    wchar_t *path = const_cast<wchar_t*>(reinterpret_cast<const wchar_t*>(programPath));
    wchar_t *args = const_cast<wchar_t*>(reinterpret_cast<const wchar_t*>(arguments));
    uint32_t pathLen = wcslen(path);
    uint32_t argsLen = (0 != args) ? wcslen(args) : 0;
    wchar_t *cmdLine = new wchar_t[pathLen + argsLen + 2];

    wcscpy_s(cmdLine, pathLen + argsLen + 2, path);
    if (0 < argsLen)
    {
        if (cmdLine != wcscat(cmdLine, L" "))
        {
            delete [] cmdLine;
            throw std::runtime_error("Process::create: can't create command line.");
        }
        if (cmdLine != wcscat(cmdLine, args))
        {
            delete [] cmdLine;
            throw std::runtime_error("Process::create: can't create command line.");
        }
    }

    BOOL success;
    PROCESS_INFORMATION processInfo;

    SetLastError(0);
#if defined(_WIN32)
    STARTUPINFO startupInfo;
    GetStartupInfo(&startupInfo);
    success = CreateProcess(0, cmdLine, 0, 0, false, 0, 0, 0, &startupInfo, &processInfo);
#else
    success = CreateProcess(path, args, 0, 0, false, 0, 0, 0, 0, &processInfo);
#endif // #if defined(_WIN32)

    delete [] cmdLine;
    cmdLine = 0;

    if (!success)
        throw_error("Process::create: CreateProcess failed: ");

    CloseHandle(processInfo.hThread);
    //We must keep the handle for calling WaitForSingleObject and GetExitCodeProcess.
    myProcHandle = processInfo.hProcess;
}

#else

void Process::create(char const* programPath, char const* arguments)
{
    if ((0 == programPath) || (strlen(programPath) < 1))
        throw std::runtime_error("Process::create: path to executable file is empty.");

    if (myProcHandle != INVALID_HANDLE_VALUE)
        CloseHandle(myProcHandle);

    BOOL success;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    std::string cmdLine(programPath);

    if ((0 != arguments) && (0 < strlen(arguments)))
    {
        cmdLine += " ";
        cmdLine += arguments;
    }

    GetStartupInfo(&startupInfo);
    SetLastError(0);
    success = CreateProcess(0, const_cast<char*>(cmdLine.c_str()), 0, 0,
                            false, 0, 0, 0, &startupInfo, &processInfo);
    if (!success)
        throw_error("Process::create: CreateProcess failed: ");

    CloseHandle(processInfo.hThread);
    //We must keep the handle for calling WaitForSingleObject and GetExitCodeProcess.
    myProcHandle = processInfo.hProcess;
}

#endif // #ifdef UNICODE

Process::ExitStatusType Process::wait()
{
    DWORD res = WaitForSingleObject(myProcHandle, INFINITE);
    if (res == WAIT_FAILED)
        throw_error("Process::kill: WaitForSingleObject failed: ");

    if (res == WAIT_TIMEOUT)
        throw std::runtime_error("Process::kill: WaitForSingleObject bad result WAIT_TIMEOUT");

    if ( !GetExitCodeProcess(myProcHandle, &res))
        throw_error("Process::kill: GetExitCodeProcess failed: ");

    if (res == STILL_ACTIVE)
        throw std::logic_error("Process::kill: exit code STILL_ACTIVE");

    return std::make_pair((res < 256) == true, res);
}

void Process::kill()
{
    if( !TerminateProcess(myProcHandle, ProcessTerminated))
        throw_error("Process::kill: TerminateProcess failed: ");
}

void Process::kill(ProcessId pid)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (h == NULL)
        throw_error("Process::kill(pid): OpenProcess failed: ");

    if( !TerminateProcess(h, ProcessTerminated))
        throw_error("Process::kill(pid): TerminateProcess failed: ");
}

Process::ProcessId Process::get_pid()
{
    return GetCurrentProcessId();
}

void Process::throw_error(std::string const& errorMessage)
{
    std::string systemErrorMessage("unknown error");
    DWORD error = GetLastError();
    if (0 != error)
        get_error_message(error, systemErrorMessage);
    systemErrorMessage = errorMessage + systemErrorMessage;
    throw std::runtime_error(systemErrorMessage);
}

void Process::get_error_message(DWORD error, std::string& errorMessage)
{
    LPTSTR messageBuffer;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &messageBuffer, 0, NULL);

#ifdef UNICODE
    static const int32_t asciiMaxLength(255);
    char asciiMessageBuffer[asciiMaxLength] = {0};
    errno_t asciiErrno;

    asciiErrno = wcstombs(asciiMessageBuffer, messageBuffer, asciiMaxLength - 1);
    if (!asciiErrno)
        errorMessage = asciiMessageBuffer;
    else
        errorMessage = "unknown error";
#else
    errorMessage = messageBuffer;
#endif
    LocalFree(messageBuffer);
}

#endif // #ifdef __linux__
