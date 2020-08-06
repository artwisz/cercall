/*!
 * \file
 * \brief     Cercall Transport interface
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

#ifndef CERCALL_TRANSPORT_H
#define CERCALL_TRANSPORT_H

#include <stdint.h>
#include <memory>
#include "cercall/cercall.h"

namespace cercall {

/**
 * @brief Transport interface.
 * @ingroup cercall
 */
class Transport : public std::enable_shared_from_this<Transport>
{
public:
    /**
     * @brief Transport listener.
     * A transport object delivers notifications to its listener.
     */
    class Listener
    {
    public:
        virtual ~Listener() {}
        /**
         * @brief Connection error notification.
         * The transport is closed by the implementation after it calls this notification.
         */
        virtual void on_connection_error(Transport&, const Error&) = 0;
        /**
         * @brief Connection notification.
         * Optional.
         */
        virtual void on_connected(Transport&)  {}
        /**
         * @brief Disconnection notification.
         */
        virtual void on_disconnected(Transport&) = 0;
        /**
         * @brief Notification about received data from the transport stream.
         * @param dataLenInBuffer length of data available in the transport input buffer
         * @return number of bytes read from the transport input buffer
         */
        virtual uint32_t on_incoming_data(Transport& tr, uint32_t dataLenInBuffer) = 0;
    };

    virtual ~Transport() noexcept(false) {}

    /**
     * Set the listener of the Transport object.
     * The listener must be set before the open() function is called.
     * @param l the Listener object
     */
    void set_listener(Listener& l)  {  myListener = &l;    }

    /**
     * Clear the listener.
     * The listener has to call it in its destructor to decouple itself from the Transport.
     */
    void clear_listener() {  myListener = nullptr; }

    /**
     * @return true if the Transport is in open state, otherwise false.
     */
    virtual bool is_open() = 0;

    /**
     * @brief Open the transport connection to a service.
     * The function blocks until the operation is completed.
     * @return true on success, otherwise false.
     */
    virtual bool open() = 0;

    /**
     * @brief Open the transport connection to a service.
     * The function is asynchronous, it starts the connect operation and returns immediately.
     * @param cl a closure which is called when the connect operation is completed, the closure
     * parameter indicates success (true) or failure (false). Additionally, on failure,
     * the closure's Result<bool> parameter contains the error code.
     * If the transport is already opened, the closure will be called immediately from this function
     * to indicate failure.
     */
    virtual void open(const cercall::Closure<bool>& cl) = 0;

    /**
     * @brief Close the transport connection.
     */
    virtual void close() = 0;

    /**
     * @brief Read len bytes from the transport channel asynchronously.
     * The transport shall prepare a buffer of at least 'len' size and read
     * at least len bytes from the channel in the backgroud.
     * The function shall start the read operation and return immediately.
     * The completion of the operation is communicated by the on_incoming_data callback.
     */
    virtual void read(uint32_t len) = 0;

    /**
     * @brief Get read data.
     * The length of received data is passed in the on_incoming_data callback.
     * It may be greater than the length requested in the read() parameter.
     * Calling this function shall move the read position of the input buffer by the number of bytes
     * requested previously in the read() function.
     * If called multiple times, the function shall return a reference to the same read data,
     * until a new read operation is requested by calling read().
     * @return reference to the buffer holding the read data.
     */
    virtual const std::string& get_read_data() = 0;

    /**
     * @brief Write data to the transport channel.
     * @param msg the data to write
     * @return an Error value, which may indicate that the operation failed.
     * The write operation may be implemented as synchronous or asynchronous.
     * Synchronous implementation is usually sufficient for local IPC communication.
     */
    virtual Error write(const std::string& msg) = 0;

protected:
    Listener* myListener = nullptr;
};

}   //namespace cercall

#endif // CERCALL_TRANSPORT_H
