/*!
 * \file
 * \brief     Configuration of asio for CerCall
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

#ifndef CERCALL_ASIO_CONFIG_H
#define CERCALL_ASIO_CONFIG_H

#ifdef ASIO_STANDALONE
#include "asio.hpp"
#else
#include "boost/asio.hpp"
#endif

#ifndef ASIO_STANDALONE
namespace asio = boost::asio;
#endif

#endif //CERCALL_ASIO_CONFIG_H
