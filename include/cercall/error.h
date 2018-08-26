/*!
 * \file
 * \brief     Cercall Error class
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

#ifndef CERCALL_ERROR_H
#define CERCALL_ERROR_H

#include <system_error>

namespace cercall {

class Error
{
public:

    explicit Error() : myNetLibCode(0) {}

    template<typename EC>
    explicit Error(const EC& ec) : myNetLibCode(ec.value()), myCategory(&ec.category())
    {
        if (ec) {
            myErrorMessage = ec.message();
        }
    }

    explicit Error(const int netLibErrorCode, const std::string& errorMessage)
    : myNetLibCode(netLibErrorCode), myErrorMessage(errorMessage)
    {
    }

    Error(const Error& other)
    {
        operator= (other);
    }

    void operator= (const Error& other)
    {
        myNetLibCode = other.myNetLibCode;
        myErrorMessage = other.myErrorMessage;
        myCategory = other.myCategory;
    }

    explicit operator bool() const noexcept
    {
        return myNetLibCode != 0 ? true : false;
    }

    int code() const {  return myNetLibCode; }

    const std::string& message() const { return myErrorMessage; }

    template<typename CategoryT>
    const CategoryT& category() const { return *static_cast<const CategoryT*>(myCategory); }

    /**
     * Create the "operation in progress" error.
     */
    static const Error& operation_in_progress();

private:
    int myNetLibCode;  ///< 0 means no error
    const void *myCategory = nullptr;
    std::string myErrorMessage;

    friend std::ostream & operator<<(std::ostream &os, const Error& p);
};

inline
std::ostream & operator<<(std::ostream &os, const Error& e)
{
    return os << e.message();
}

}   //namespace cercall

#endif // CERCALL_ERROR_H

