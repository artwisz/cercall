/*!
 * \file
 * \brief     Cercall tests - Calculator client class
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

#ifndef CERCALL_CALCULATORCLIENT_H
#define CERCALL_CALCULATORCLIENT_H

#include "calculatorinterface.h"
#include "cercall/client.h"

static constexpr std::size_t ClientCallQueueSize = 3U;

template<class SerializationType>
class CalculatorClient : public cercall::Client<CalculatorInterface, SerializationType, ClientCallQueueSize>
{
    using MyBase = cercall::Client<CalculatorInterface, SerializationType, ClientCallQueueSize>;
public:
    CalculatorClient(std::unique_ptr<cercall::Transport> tr) : MyBase(std::move(tr)) {}

    void add(int8_t a, int16_t b, int32_t c, cercall::Closure<int32_t> cl) override
    {
        this->send_call(__func__, cl, a, b, c);
    }

    void add_and_delay_result(int32_t a, int32_t b, cercall::Closure<int32_t> cl) override
    {
        this->send_call(__func__, cl, a, b);
    }

    void add_vector(const std::vector<int32_t>& a, const std::vector<int32_t>& b,
                    cercall::Closure<std::vector<int64_t>> cl) override
    {
        this->send_call(__func__, cl, a, b);
    }

#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
    void add_by_pointers(std::unique_ptr<int32_t> a, std::unique_ptr<int32_t> b, cercall::Closure<int32_t> cl) override
    {
        this->send_call(__func__, cl, std::move(a), std::move(b));
    }
#endif

    void close_service(void) override
    {
        this->send_call(__func__);
    }

    void get_connected_clients_count(cercall::Closure<size_t> cl)
    {
        this->send_call(__func__, cl);
    }
};

#endif //CERCALL_CALCULATORCLIENT_H
