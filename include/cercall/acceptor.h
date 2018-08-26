/*!
 * \file
 * \brief     Cercall Acceptor interface
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

#ifndef CERCALL_ACCEPTOR_H
#define CERCALL_ACCEPTOR_H

#include <memory>
#include "cercall/transport.h"

namespace cercall {

/**
 * @brief An Acceptor interface.
 * The Acceptor opens a server endpoint and accepts connecting clients.
 * @ingroup cercall
 */
class Acceptor
{
public:

    /**
     * @brief Acceptor listener.
     * An Acceptor object delivers notifications to its listener.
     */
    class Listener
    {
    public:
        virtual ~Listener() {}
        /**
         * Notifies the listener about a newly accepted client connection.
         * @param clientTransport the accepted client transport
         */
        virtual void on_client_accepted(std::shared_ptr<Transport> clientTransport) = 0;
        /**
         * Notifies the listener about an error.
         * @param e the error code.
         */
        virtual void on_accept_error(const Error& e) = 0;
    };

    virtual ~Acceptor() {}

    /**
     * Set the listener of the Acceptor object.
     * The listener must be set before open() is called.
     * @param l the listener object
     */
    void set_listener(Listener& l)  {  myListener = &l;    }

    /**
     * @return true if the Acceptor is in open state, otherwise false.
     */
    virtual bool is_open() const = 0;

    /**
     * @brief Open the server endpoint and start accepting clients.
     * @param maxPendingClientConnections when greater than 0, the the maximum number of accepted
     *        queued client connections, 0 and a negative value is ignored - the default is in effect then
     */
    virtual void open(int maxPendingClientConnections) = 0;

    /**
     * @brief Close the server endpoint, clients cannot connect any more.
     */
    virtual void close() = 0;

protected:
    Listener* myListener = nullptr;
};

}   //namespace cercall

#endif // ACCEPTOR_H
