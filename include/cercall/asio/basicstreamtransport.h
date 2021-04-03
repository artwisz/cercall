/*!
 * \file
 * \brief     Cercall Transport common base class for Asio stream sockets
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


#ifndef CERCALL_ASIO_BASICSTREAMTRANSPORT_H
#define CERCALL_ASIO_BASICSTREAMTRANSPORT_H

#include "cercall/transport.h"
#include "cercall/asio/errorcode.h"
#include "cercall/log.h"
#include <deque>
#include <memory>
#include <functional>
#include <array>
#include <iostream>

namespace cercall {
namespace asio {

template<class StreamProtocol>
class BasicStreamTransport : public Transport
{
public:
    using SocketType = std::shared_ptr <typename StreamProtocol::socket>;

    BasicStreamTransport(::asio::io_service& ios) : mySocket { std::make_shared<typename StreamProtocol::socket>(ios) }
    {
        log<trace>(O_LOG_TOKEN, "io_service param");
    }

    /** Construct from an accepted socket. */
    BasicStreamTransport(SocketType s) : mySocket(s)
    {
        log<trace>(O_LOG_TOKEN, "socket param");
    }
    BasicStreamTransport(const BasicStreamTransport&) = delete;
    BasicStreamTransport& operator=(const BasicStreamTransport&) = delete;
    virtual ~BasicStreamTransport() noexcept(false)
    {
        log<trace>(O_LOG_TOKEN, "");
        if (myState != State::CLOSED) {
            close();
        }
    }

    bool is_open() override
    {
        o_assert(mySocket);
        return myState == State::OPEN;
    }

    virtual void set_socket_options(typename StreamProtocol::socket&) {}

    void read(uint32_t len) override
    {
        myBuffer.resize(len);
        auto sharedThis = std::static_pointer_cast<BasicStreamTransport<StreamProtocol>>(this->shared_from_this());
        ::asio::async_read(*mySocket, ::asio::buffer(&myBuffer[0], len),
                           std::bind(&BasicStreamTransport::handle_recv, sharedThis,
                                     std::placeholders::_1, std::placeholders::_2));
    }

    const std::string& get_read_data() override
    {
        return myBuffer;
    }

    Error write(const std::string& msg) override
    {
        Error result;
        if (is_open()) {
            ErrorCode ec;
            ::asio::write(*mySocket, ::asio::buffer(msg), ec);
            if (ec)
            {
                log<error>(O_LOG_TOKEN, "write error - %s", ec.message().c_str());
                close();
                result = Error (ec);
            }
        } else {
            result = Error { ENOTCONN, "socket not connected" };
        }
        return result;
    }

    void close() override
    {
        log<trace>(O_LOG_TOKEN, "");
        if (myState == State::OPEN)
        {
            myState = State::CLOSED;
            log<debug>(O_LOG_TOKEN, "shutdown socket");
            ErrorCode ec;
            mySocket->shutdown(::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec) {
                log<error>(O_LOG_TOKEN, "shutdown error - %s", ec.message().c_str());
            }
            mySocket->close();
            if (myListener) {
                myListener->on_disconnected(*this);
            }
        }
    }

protected:

    enum class State
    {
        NEW, OPEN, CLOSED
    };

    State myState = State::NEW;
    SocketType mySocket;

    bool open() override
    {
        log<trace>(O_LOG_TOKEN, "");
        o_assert(myListener != nullptr);
        o_assert(mySocket);
        o_assert(myState == State::NEW);
        myState = State::OPEN;
        set_socket_options(*mySocket);
        myListener->on_connected(*this);
        return true;
    }

private:

    std::string myBuffer;

    void handle_recv(const ErrorCode& ec, std::size_t bytesTransferred)
    {
        if (ec) {
            if ( !(::asio::error::operation_aborted == ec && myState == State::CLOSED)) {
                log<error>(O_LOG_TOKEN, "error - %s", ec.message().c_str());
                if (myListener != nullptr) {
                    myListener->on_connection_error(*this, Error(ec));
                }
            }
            if ( !is_recoverable(ec.value())) {
                close();
            }
        } else {
            o_assert(myListener != nullptr);
            myListener->on_incoming_data(*this, bytesTransferred);
        }
    }

    /**
     * @return true if the network error netLibCode is recoverable (the socket need not be closed)
     */
    bool is_recoverable(int netLibCode)
    {
        return netLibCode == EAGAIN || netLibCode == EWOULDBLOCK || netLibCode == EINTR;
    }

    /** Not to be used, but to be overriden by derived classes. */
    void open(const cercall::Closure<bool>&) override
    {
    }

};

}   //namespace asio
}   //namespace cercall

#endif // CERCALL_ASIO_BASICSTREAMTRANSPORT_H
