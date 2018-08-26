/*!
 * \file
 * \brief     error_code declaration for asio
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

#ifndef CERCALL_ASIO_ERROR_CODE_H
#define CERCALL_ASIO_ERROR_CODE_H

#include "cercall/asio/config.h"
#include "cercall/details/cpputil.h"
#include "cercall/error.h"

namespace cercall {
namespace asio {

#ifdef ASIO_STANDALONE
#include <asio/error.hpp>
#include <system_error>

using ErrorCode = std::error_code;
using ErrorCategory = std::error_category;

inline const ErrorCategory& system_category()
{
    return std::system_category();
}

}   //namespace asio

#else

#include <boost/asio/error.hpp>

using ErrorCode = ::boost::system::error_code;
using ErrorCategory = ::boost::system::error_category;

inline const ErrorCategory& system_category()
{
    return ::boost::system::system_category();
}

}   //namespace asio

#endif  //ASIO_STANDALONE

const Error& Error::operation_in_progress()
{
    static std::unique_ptr<Error> err = cercall::make_unique<Error>(asio::ErrorCode(EINPROGRESS, asio::system_category()));
    return *err;
}

}   //namespace cercall

#endif // CERCALL_ASIO_ERROR_CODE_H
