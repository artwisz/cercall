/*!
 * \file
 * \brief     Cercall Acceptor common base class for Asio stream sockets
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

#ifndef CERCALL_ASIO_BASICSTREAMACCEPTOR_H
#define CERCALL_ASIO_BASICSTREAMACCEPTOR_H

#include "cercall/acceptor.h"
#include "cercall/asio/basicstreamtransport.h"
#include "cercall/log.h"

namespace cercall {
namespace asio {

template<class StreamProtocol>
class BasicStreamAcceptor : public Acceptor
{
public:

    using EndpointType = typename StreamProtocol::endpoint;
    using SocketType = typename StreamProtocol::socket;
    typedef std::shared_ptr <SocketType> SocketPtr;
    using TransportType = BasicStreamTransport<StreamProtocol>;

    BasicStreamAcceptor(::asio::io_service& ios, EndpointType endp)
        : myIoService(ios), myEndpoint(endp), myAcceptor(ios)
    {
        log<trace>(O_LOG_TOKEN, "");
    }

    virtual ~BasicStreamAcceptor() noexcept(false)
    {
        log<trace>(O_LOG_TOKEN, "");
        if (myAcceptor.is_open()) {
            myAcceptor.close();
        }
    }

    virtual void set_socket_options(typename StreamProtocol::acceptor& acceptor) = 0;

    bool is_open() const override
    {
        return myAcceptor.is_open();
    }

    void open(int maxPendingClientConnections = -1) override
    {
        log<trace>(O_LOG_TOKEN, "");
        if (myListener == nullptr) {
            throw std::logic_error("cercall::asio::BasicStreamAcceptor: listener is NULL");
        }
        if ( !myAcceptor.is_open()) {
            ErrorCode ec;
            log<debug>(O_LOG_TOKEN, "open acceptor");
            myAcceptor.open(myEndpoint.protocol(), ec);
            if (ec) {
                log<error>(O_LOG_TOKEN, "open error - %s", ec.message().c_str());
                myListener->on_accept_error(Error(ec));
                return;
            }
            set_socket_options(myAcceptor);
            log<debug>(O_LOG_TOKEN, "bind endpoint");
            myAcceptor.bind(myEndpoint, ec);
            if (ec) {
                log<error>(O_LOG_TOKEN, "bind error - %s", ec.message().c_str());
                myListener->on_accept_error(Error(ec));
                return;
            }
            log<debug>(O_LOG_TOKEN, "listen");
            myAcceptor.listen((maxPendingClientConnections > 0) ? maxPendingClientConnections
                                                                : StreamProtocol::socket::max_connections, ec);
            if (ec) {
                log<error>(O_LOG_TOKEN, "listen error - %s", ec.message().c_str());
                myListener->on_accept_error(Error(ec));
            }
        }
        start_accept();
    }

    void close() override
    {
        if (is_open()) {
            log<debug>(O_LOG_TOKEN, "close acceptor");
            myAcceptor.close();
        }
    }

private:

    ::asio::io_service& myIoService;
    EndpointType myEndpoint;
    typename StreamProtocol::acceptor myAcceptor;

    void start_accept()
    {
        o_assert(myListener != nullptr);
        SocketPtr socket { new SocketType(myIoService) };
        myAcceptor.async_accept(*socket, [this, socket] (const ErrorCode& ec) {
            if ( !ec && is_open()) {
                std::shared_ptr<TransportType> clientTr { new TransportType(socket) };
                myListener->on_client_accepted(clientTr);
            } else if (is_open() || ec != ::asio::error::operation_aborted) {
                log<error>(O_LOG_TOKEN, "async_accept error - %s", ec.message().c_str());
                myListener->on_accept_error(cercall::Error(ec));
            }
            if (is_open()) {
                start_accept();
            }
        });
    }
};

}   //namespace asio
}   //namespace cercall

#endif // CERCALL_ASIO_BASICSTREAMACCEPTOR_H

