/*!
 * \file
 * \brief     Cercall tests - testing function calls
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

#include "debug.h"
#include "cercall/cercall.h"
#include "cercall/asio/tcpacceptor.h"
#include "calculatorservice.h"
#include <algorithm>
#include "testutil.h"

static asio::io_service ioService;
static std::shared_ptr<asio::io_service::work> iosWork;

void signal_handler(const std::error_code&, int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        //std::cout << "got signal to finish program\n";
        iosWork.reset();
        ioService.stop();
    }
}

int main(int ac, char *av[])
{
    asio::signal_set signals(ioService, SIGINT, SIGTERM);
    signals.async_wait(signal_handler);

    try {
        auto acceptor = cercall::make_unique<cercall::asio::TcpAcceptor>(ioService, TEST_SERVICE_PORT);
        auto closeAction = [](){
                iosWork.reset();
                ioService.stop();
            };
        using CalculatorServiceType = CalculatorService<CalculatorInterface::Serialization>;
        std::shared_ptr<CalculatorServiceType> service = std::make_shared<CalculatorServiceType>(ioService, std::move(acceptor),
                                                                                         closeAction);
        iosWork = std::make_shared<asio::io_service::work>(ioService);

        service->start();
        if (ac > 1 && std::string(av[1]) == "-t") {
            cercall::log<cercall::debug>(O_LOG_TOKEN, "connection reset test");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            service->stop();
        } else {
            ioService.run();
        }
    } catch (std::exception& e) {
        cercall::log<cercall::error>(O_LOG_TOKEN, "Exception: %s", e.what());
    }
    cercall::log<cercall::debug>(O_LOG_TOKEN, "exit calltestserver now");
    return 0;
}

