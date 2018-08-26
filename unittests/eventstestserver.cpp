/*!
 * \file
 * \brief     Cercall tests - testing events
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
#include "simpleeventsourceinterface.h"
#include "polyeventsourceinterface.h"
#include "cercall/asio/tcpacceptor.h"
#include <algorithm>
#include "testutil.h"

static asio::io_service ioService;
static std::shared_ptr<asio::io_service::work> iosWork;

static constexpr std::chrono::milliseconds EventTickInterval { 100 };

class SimpleEventSourceService : public cercall::Service<SimpleEventSourceInterface, SimpleEventSourceInterface::Serialization>,
                                 public std::enable_shared_from_this<SimpleEventSourceService>
{
public:
    SimpleEventSourceService(asio::io_service& ios, std::unique_ptr<cercall::Acceptor> ac)
        : cercall::Service<SimpleEventSourceInterface, SimpleEventSourceInterface::Serialization>(std::move(ac)), myEventTimer(ios)
    {
        O_ADD_SERVICE_FUNCTIONS_OF(SimpleEventSourceInterface, true, trigger_single_broadcast);
    }

    void trigger_single_broadcast(EventType et) override
    {
        myEnabledEvent = et;
        if (myEnabledEvent != EventType::NO_EVENT) {
            myEventTimer.expires_from_now(EventTickInterval);
            myEventTimer.async_wait(std::bind(&SimpleEventSourceService::tickTimer, shared_from_this(), std::placeholders::_1));
        }
    }
private:

    asio::steady_timer myEventTimer;
    EventType myEnabledEvent;

    void tickTimer(const cercall::asio::ErrorCode& ec)
    {
        if (ec != asio::error::operation_aborted && myEnabledEvent != EventType::NO_EVENT) {
            broadcast_event(myEnabledEvent);
        }
    }
};

class PolyEventSourceService : public cercall::Service<PolyEventSourceInterface>,
                               public std::enable_shared_from_this<PolyEventSourceService>
{
public:
    PolyEventSourceService(asio::io_service& ios, std::unique_ptr<cercall::Acceptor> ac)
        : cercall::Service<PolyEventSourceInterface>(std::move(ac)), myEventTimer(ios)
    {
        O_ADD_SERVICE_FUNCTIONS_OF(PolyEventSourceInterface, true, trigger_single_broadcast);
    }

    void trigger_single_broadcast(const std::shared_ptr<EventType>& e) override
    {
        myEnabledEvent = e;
        if (myEnabledEvent.get() != nullptr) {
            myEventTimer.expires_from_now(EventTickInterval);
            myEventTimer.async_wait(std::bind(&PolyEventSourceService::tickTimer, shared_from_this(), std::placeholders::_1));
        }
    }
private:

    asio::steady_timer myEventTimer;
    std::shared_ptr<EventType> myEnabledEvent;

    void tickTimer(const cercall::asio::ErrorCode& ec)
    {
        if (ec != asio::error::operation_aborted && myEnabledEvent.get() != nullptr) {
            std::string eventClassName;
            if (myEnabledEvent->get_as<RealEventClassOne>() != nullptr) {
                eventClassName = "RealEventClassOne";
            } else if (myEnabledEvent->get_as<RealEventClassThree>() != nullptr) {  //check before RealEventClassTwo!
                eventClassName = "RealEventClassThree";
            } else if (myEnabledEvent->get_as<RealEventClassTwo>() != nullptr) {
                eventClassName = "RealEventClassTwo";
            }
            std::cout << "broadcast " << eventClassName << '\n';
            broadcast_event(myEnabledEvent);
        }
    }
};

void signal_handler(const std::error_code&, int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        iosWork.reset();
        ioService.stop();
    }
}

template<typename Service>
void run_service(void)
{
    try {
        iosWork = std::make_shared<asio::io_service::work>(ioService);
        auto acceptor = cercall::make_unique<cercall::asio::TcpAcceptor>(ioService, TEST_SERVICE_PORT);
        std::shared_ptr<Service> service = std::make_shared<Service>(ioService, std::move(acceptor));

        service->start();
        ioService.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

int main(int ac, char const * av[])
{
    asio::signal_set signals(ioService, SIGINT, SIGTERM);
    signals.async_wait(signal_handler);

    if (ac < 2) {
        std::cerr << "program argument: -s or -p\n";
    } else {
        std::string arg { av[1] };

        if (arg == "-s") {
            run_service<SimpleEventSourceService>();
        } else if (arg == "-p") {
            run_service<PolyEventSourceService>();
        } else {
            std::cerr << "invalid argument\n";
            return 2;
        }
    }

    return 0;
}
