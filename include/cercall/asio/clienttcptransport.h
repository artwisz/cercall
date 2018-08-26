/*!
 * \file
 * \brief     Cercall client TCP Transport for Asio stream sockets
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

#ifndef CERCALL_ASIO_CLIENTTCPTRANSPORT_H
#define CERCALL_ASIO_CLIENTTCPTRANSPORT_H

#include "cercall/asio/tcptransport.h"
#include "cercall/cercall.h"

namespace cercall {
namespace asio {

class ClientTcpTransport : public TcpTransport
{
public:
    ClientTcpTransport(::asio::io_service& ios,
                       const std::string& hostName,
                       const std::string& serviceName)
        : TcpTransport(ios), myIoService (ios), myHostName (hostName), myServiceName(serviceName), myResolver(ios) {}

    bool open() override
    {
        using ::asio::ip::tcp;

        if (myState == State::NEW) {
            tcp::resolver::query query(tcp::v4(), myHostName, myServiceName, tcp::resolver::query::canonical_name);
            tcp::resolver::iterator iterator = myResolver.resolve(query);

            ErrorCode ec;
            ::asio::connect(*mySocket, iterator, ec);
            if (ec) {
                myListener->on_connection_error(*this, Error(ec));
                return false;
            } else {
                return TcpTransport::open();
            }
        } else {
            const char* stateStr = (myState == State::OPEN) ? "OPEN" : "CLOSED";
            log<error>("ClientTcpTransport", __func__, "can't open transport in %s state", stateStr);
            return false;
        }
    }

    void open(const cercall::Closure<bool>& cl) override
    {
        using ::asio::ip::tcp;

        o_assert(myListener != nullptr);

        if (myState == State::NEW) {
            tcp::resolver::query query(tcp::v4(), myHostName, myServiceName, tcp::resolver::query::canonical_name);

            auto sharedThis = std::static_pointer_cast<ClientTcpTransport>(this->shared_from_this());
            myResolver.async_resolve(query, std::bind(&ClientTcpTransport::handle_resolve, sharedThis, cl,
                                                    std::placeholders::_1, std::placeholders::_2));
        } else {
            const char* stateStr = (myState == State::OPEN) ? "OPEN" : "CLOSED";
            log<error>(O_LOG_TOKEN, "can't open transport in %s state", stateStr);
            Result<bool> result { false, Error { std::make_error_code(std::errc::already_connected) } };
            cl(result);
        }
    }

private:

    void handle_resolve(const cercall::Closure<bool>& cl, const ErrorCode& ec, ::asio::ip::tcp::resolver::iterator i)
    {
        if (ec)  {
            log<error>(O_LOG_TOKEN, "resolve error - %s", ec.message().c_str());
            myListener->on_connection_error(*this, Error { ec });
            cl(Result<bool> { false, Error { ec } });
        } else {
            auto sharedThis = std::static_pointer_cast<ClientTcpTransport>(this->shared_from_this());
            ::asio::async_connect(*mySocket, i, std::bind(&ClientTcpTransport::handle_connect, sharedThis, cl,
                                                          std::placeholders::_1, std::placeholders::_2));
        }
    }

    void handle_connect(const cercall::Closure<bool>& cl, const ErrorCode& ec, ::asio::ip::tcp::resolver::iterator)
    {
        if (ec) {
            log<error>(O_LOG_TOKEN, "connect error - %s", ec.message().c_str());
            myListener->on_connection_error(*this, Error { ec });
            cl(Result<bool> { false, Error { ec } });
        } else {
            bool result = TcpTransport::open();
            cl(Result<bool> { result });
        }
    }

    ::asio::io_service& myIoService;
    std::string myHostName;
    std::string myServiceName;
    ::asio::ip::tcp::resolver myResolver;
};

}   //namespace asio
}   //namespace cercall

#endif // CERCALL_ASIO_CLIENTTCPTRANSPORT_H

