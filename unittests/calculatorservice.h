/*!
 * \file
 * \brief     Cercall tests - Calculator service class
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


#include "cercall/cercall.h"
#include "cercall/service.h"
#include "calculatorinterface.h"

template<class SerializationType>
class CalculatorService : public cercall::Service<CalculatorInterface, SerializationType>
{
public:
    CalculatorService(asio::io_service& ios, std::unique_ptr<cercall::Acceptor> ac,
                      std::function<void()> serviceCloseAction)
        : cercall::Service<CalculatorInterface, SerializationType>(std::move(ac)), myResultTimer(ios),
        myServiceCloseAction(serviceCloseAction)
    {
        O_ADD_SERVICE_FUNCTIONS_OF(CalculatorInterface, false, add, add_vector, add_and_delay_result);
        O_ADD_SERVICE_FUNCTIONS_OF(CalculatorInterface, false, get_connected_clients_count);
#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
        O_ADD_SERVICE_FUNCTIONS_OF(CalculatorInterface, false, add_by_pointers);
#endif
        O_ADD_SERVICE_FUNCTIONS_OF(CalculatorInterface, true, close_service);
    }

    void add(int8_t a, int16_t b, int32_t c, cercall::Closure<int32_t> cl) override
    {
        int64_t sum = (int64_t)a +(int64_t)b + (int64_t)c;
        if (sum > INT32_MAX) {
            cercall::Result<int32_t> result { cercall::Error(std::make_error_code(std::errc::value_too_large)) };
            cl(result);
        } else {
            cl(static_cast<int32_t>(sum));
        }
    }

    void add_and_delay_result(int32_t a, int32_t b, cercall::Closure<int32_t> cl) override
    {
        //std::cout << "add_and_delay_result call\n";
        myResultTimer.expires_from_now(std::chrono::seconds(1));
        //shared_from_this() not used on purpose.
        myResultTimer.async_wait([a, b, cl](const std::error_code&){
            //std::cout << "add_and_delay_result pass result\n";
           cl(a + b);
        });
    }

    void add_vector(const std::vector<int32_t>& a, const std::vector<int32_t>& b,
                    cercall::Closure<std::vector<int64_t>> cl) override
    {
        std::vector<int64_t> result;
        if (a.size() == b.size()) {
            result.resize(a.size());
            std::transform (a.begin(), a.end(), b.begin(), result.begin(), std::plus<int64_t>());
        }
        cl(result);
    }

#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
    void add_by_pointers(std::unique_ptr<int32_t> a, std::unique_ptr<int32_t> b, cercall::Closure<int32_t> cl) override
    {
        cl(*a + *b);
    }
#endif

    void close_service(void) override
    {
        myServiceCloseAction();
    }

    void get_connected_clients_count(cercall::Closure<size_t> cl) override
    {
        std::size_t numConnected = this->get_clients().size();
        cercall::log<cercall::debug>(O_LOG_TOKEN, "%d connected clients", numConnected);
        cl(numConnected);
    }

private:
    asio::steady_timer myResultTimer;
    std::function<void()> myServiceCloseAction;
};
