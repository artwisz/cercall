/*!
 * \file
 * \brief     Cercall example - Clock service class
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

#include "cercall/asio/tcpacceptor.h"
#include "clockservice.h"

static ClockAlarmId nextAlarmId = 1U;

ClockService::ClockService(asio::io_service& ios, std::unique_ptr<cercall::Acceptor> ac)
    : Service<ClockInterface, BinarySerialization>(std::move(ac)), myIoService(ios), myTickTimer(ios)
{
    O_ADD_SERVICE_FUNCTIONS_OF(ClockInterface, false, get_time, set_tick_interval, set_alarm, cancel_alarm);
    O_ADD_SERVICE_FUNCTIONS_OF(ManagableServiceInterface, false, close_service);
}

void ClockService::get_time(cercall::Closure<std::chrono::system_clock::time_point> closure)
{
    closure(std::chrono::system_clock::now());
}

void ClockService::tickTimer(const cercall::asio::ErrorCode& ec)
{
    if (ec != asio::error::operation_aborted) {
        broadcast_event<ClockTickEvent>(std::chrono::system_clock::now());
        if (myTickInterval != std::chrono::milliseconds::zero()) {
            myTickTimer.expires_at(myTickTimer.expires_at() + myTickInterval);
            myTickTimer.async_wait(std::bind(&ClockService::tickTimer, shared_from_this(), std::placeholders::_1));
        }
    }
}

void ClockService::set_tick_interval(std::chrono::milliseconds tickInterval, cercall::Closure<void> closure)
{
    myTickInterval = tickInterval;
    myTickTimer.cancel();

    if (tickInterval != std::chrono::milliseconds::zero()) {
        myTickTimer.expires_from_now(tickInterval);
        auto shared_this = shared_from_this();
        myTickTimer.async_wait(std::bind(&ClockService::tickTimer, shared_from_this(), std::placeholders::_1));
    }

    closure();
}

auto ClockService::find_alarm(ClockAlarmId alarmId) -> std::list<ClockService::Alarm>::iterator
{
    return std::find_if(myAlarms.begin(), myAlarms.end(), [alarmId](const Alarm& a) {
                                   return a.myId == alarmId;
                               });
}

void ClockService::set_alarm(std::string tag, std::chrono::system_clock::duration after,
                             cercall::Closure<ClockAlarmId> closure)
{
    myAlarms.emplace(myAlarms.end(), nextAlarmId++, myIoService, after, tag);
    auto shared_this = shared_from_this();
    Alarm& theAlarm = myAlarms.back();
    theAlarm.myTimer.async_wait([shared_this, &theAlarm](const cercall::asio::ErrorCode& ec) {
        if (ec != asio::error::operation_aborted) {
            shared_this->broadcast_event<ClockAlarmEvent>(theAlarm.myId, theAlarm.myTag);
        }
        //Purge the alarm.
        auto it = shared_this->find_alarm(theAlarm.myId);
        assert(it != shared_this->myAlarms.end());
        shared_this->myAlarms.erase(it);
    });
    closure(theAlarm.myId);
}


void ClockService::cancel_alarm(ClockAlarmId alarm, cercall::Closure<void> closure)
{
    auto it = find_alarm(alarm);
    if (it != myAlarms.end()) {
        it->myTimer.cancel();
    }
    closure();
}

void ClockService::on_connection_error(cercall::Transport&, const cercall::Error& e)
{
    std::cerr << "client connection error: " << e.message() << '\n';
}

static asio::io_service ioService;
static std::shared_ptr<asio::io_service::work> iosWork;

void signal_handler(const std::error_code& ec, int signal_number)
{
    if (ec) {
        std::cerr << "signal_handler error " << ec.message() << '\n';
    }
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        std::cout << "terminate clock service" << std::endl;
        iosWork.reset();
        ioService.stop();
    }
}

void ClockService::close_service(const std::string &reason, cercall::Closure<int> closure)
{
    std::cout << "close service by client: " << reason << '\n';
    iosWork.reset();
    ioService.stop();
    closure(0);
}

int main(int ac, char **av)
{
    (void)ac;
    (void)av;

    asio::signal_set signals(ioService, SIGINT, SIGTERM);
    signals.async_wait(signal_handler);

    try {
        auto acceptor = std::make_unique<cercall::asio::TcpAcceptor>(ioService, 4321);
        std::shared_ptr<ClockService> service = std::make_shared<ClockService>(ioService, std::move(acceptor));
        iosWork = std::make_shared<asio::io_service::work>(ioService);

        service->start();
        std::cout << "Clock service ready\n";
        std::cout.flush();
        ioService.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
