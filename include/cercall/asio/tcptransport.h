/*!
 * \file
 * \brief     Cercall TCP Transport for Asio stream sockets
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

#ifndef CERCALL_ASIO_TCPTRANSPORT_H
#define CERCALL_ASIO_TCPTRANSPORT_H

#include "cercall/asio/basicstreamtransport.h"

namespace cercall {
namespace asio {

class TcpTransport : public BasicStreamTransport<::asio::ip::tcp>
{
public:
    using SocketType = typename BasicStreamTransport<::asio::ip::tcp>::SocketType;

    TcpTransport(::asio::io_service& ios) : BasicStreamTransport(ios) {}
    TcpTransport(SocketType s) : BasicStreamTransport(s) {}

    void set_socket_options(::asio::ip::tcp::socket& s) override
    {
        s.set_option(::asio::ip::tcp::no_delay(true));
    }

};

}   //namespace asio
}   //namespace cercall

#endif // CERCALL_ASIO_TCPTRANSPORT_H
