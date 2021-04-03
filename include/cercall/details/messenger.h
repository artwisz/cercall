/*!
 * \file
 * \brief     Cercall library details - a Messenger class
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

#ifndef CERCALL_DETAILS_MESSENGER_H
#define CERCALL_DETAILS_MESSENGER_H

#include <cstring>
#include <ostream>
#include <functional>
#include <limits>
#include <algorithm>
#include "cercall/transport.h"
#include "cercall/log.h"

namespace cercall {
namespace details {

class Messenger
{
public:
    using HandlerType = std::function<void(Transport& t, const std::string&)>;

    Messenger(HandlerType messageHandler)
        : myMsgHandler(messageHandler) {}

    Messenger(const Messenger& other)
    {
        myMsgHandler = other.myMsgHandler;
        myReceiveState = MsgRecvState::HEADER;
    }

    Messenger& operator=(const Messenger& other)
    {
        myMsgHandler = other.myMsgHandler;
        myReceiveState = MsgRecvState::HEADER;
        return *this;
    }

    void init_transport(Transport& tr)
    {
        o_assert(myReceiveState == MsgRecvState::HEADER);
        tr.read(HEADER_SIZE);
    }

    std::size_t read(Transport& tr, std::size_t dataLenInBuffer)
    {
        bool hasMoreData = true;
        std::size_t bytesRead = 0;

        while(hasMoreData) {
            switch (myReceiveState) {
            case MsgRecvState::HEADER:
                if (dataLenInBuffer >= HEADER_SIZE) {
                    const std::string& readData = tr.get_read_data();
                    o_assert(readData.length() >= HEADER_SIZE);
                    std::copy_n(readData.begin(), HEADER_SIZE, reinterpret_cast<std::string::value_type*>(&myIncomingMsgSize));
                    bytesRead += HEADER_SIZE;
                    if (myIncomingMsgSize == 0) {
                        throw std::runtime_error("invalid message length received");
                    }
                    myReceiveState = MsgRecvState::MESSAGE;
                    tr.read(myIncomingMsgSize);
                    dataLenInBuffer -= HEADER_SIZE;
                } else {
                    hasMoreData = false;
                }
                break;
            case MsgRecvState::MESSAGE:
                if (dataLenInBuffer >= myIncomingMsgSize) {
                    const std::string& msgData = tr.get_read_data();
                    o_assert(msgData.length() >= myIncomingMsgSize);
                    //log<debug>(O_LOG_TOKEN, "read msg (size=%d): %s", msgData.size(), msgData.c_str());
                    myMsgHandler(tr, msgData);
                    bytesRead += myIncomingMsgSize;
                    myReceiveState = MsgRecvState::HEADER;
                    tr.read(HEADER_SIZE);
                    dataLenInBuffer -= myIncomingMsgSize;
                } else {
                    hasMoreData = false;
                }
                break;
            default:
                o_assert("Invalid MsgRecvState value" == 0);
                break;
            }
        }
        return bytesRead;
    }

    static void reserve_message_header(std::ostream& os)
    {
        char hdrPlace[HEADER_SIZE] = { 0 };
        os.write(hdrPlace, sizeof(hdrPlace));
    }

    static Error write_message_with_header(Transport& tr, std::string& msg)
    {
        if (msg.length() > std::numeric_limits<MessageSizeType>::max() - HEADER_SIZE) {
            throw std::length_error("message too long");
        }
        MessageSizeType msgSize = static_cast<MessageSizeType>(msg.size()) - HEADER_SIZE;
        memcpy(&msg[0], &msgSize, HEADER_SIZE);
        //log<debug>(O_LOG_TOKEN, "write msg (size=%d): %s", msgSize, &msg[HEADER_SIZE]);
        return tr.write(msg);
    }

    static void strip_header(std::string& msg)
    {
        msg.erase(0, HEADER_SIZE);
    }

private:
    using MessageSizeType = std::uint32_t;      //< This could be a class template parameter.
    static constexpr std::size_t HEADER_SIZE = sizeof(MessageSizeType);
    enum class MsgRecvState    {   HEADER, MESSAGE };

    MsgRecvState myReceiveState = MsgRecvState::HEADER;
    MessageSizeType myIncomingMsgSize = 0;
    HandlerType myMsgHandler;
};

}   //namespace details
}   //namespace cercall

#endif // CERCALL_DETAILS_MESSENGER_H
