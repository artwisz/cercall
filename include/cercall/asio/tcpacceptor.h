/*!
 * \file
 * \brief     Cercall TCP Acceptor class for Asio stream sockets
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

#ifndef CERCALL_ASIO_TCPACCEPTOR_H
#define CERCALL_ASIO_TCPACCEPTOR_H

#include "cercall/asio/basicstreamacceptor.h"

namespace cercall {
namespace asio {

class TcpAcceptor : public BasicStreamAcceptor<::asio::ip::tcp>
{
public:

    TcpAcceptor(::asio::io_service& ios, unsigned short port)
        : BasicStreamAcceptor(ios, ::asio::ip::tcp::endpoint(::asio::ip::tcp::v4(), port))
    {}

    void set_socket_options(::asio::ip::tcp::acceptor& acc) override
    {
        acc.set_option(::asio::ip::tcp::acceptor::reuse_address(true));
    }
};


}   //namespace asio
}   //namespace cercall

#endif // CERCALL_ASIO_TCPACCEPTOR_H

