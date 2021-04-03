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

#ifndef CERCALL_CLOCKSERVICE_H
#define CERCALL_CLOCKSERVICE_H

#include "clockinterface.h"
#include "cercall/service.h"
#include <list>
#ifdef HAS_CEREAL
#include "cereal_setup.h"
#else
#include "boost_setup.h"
#endif

class ClockService : public cercall::Service<ClockInterface, BinarySerialization>,
                     public std::enable_shared_from_this<ClockService>
{
public:
    ClockService(asio::io_service& ios, std::unique_ptr<cercall::Acceptor> ac);

    void close_service(const std::string& reason, cercall::Closure<int> closure) override;

    void get_time(Closure<std::chrono::system_clock::time_point> closure) override;

    void set_tick_interval(std::chrono::milliseconds tickInterval, Closure<void> closure) override;

    void set_alarm(std::string tag, std::chrono::system_clock::duration after, Closure<ClockAlarmId> closure) override;

    void cancel_alarm(ClockAlarmId alarm, Closure<void> closure) override;

    void on_connection_error(cercall::Transport&, const cercall::Error&) override;

private:
    struct Alarm
    {
        asio::steady_timer myTimer;
        std::string myTag;
        ClockAlarmId myId;
        explicit Alarm(ClockAlarmId id, asio::io_service &ios, std::chrono::system_clock::duration expiryTime, std::string tag)
        : myTimer(ios, expiryTime), myTag(tag), myId(id)
        {
        }
    };

    asio::io_service& myIoService;

    std::list<Alarm> myAlarms;

    asio::steady_timer myTickTimer;

    std::chrono::milliseconds myTickInterval { 0 };

    std::list<Alarm>::iterator find_alarm(ClockAlarmId alarmId);

    void tickTimer(const cercall::asio::ErrorCode& ec);
};

#endif //CERCALL_CLOCKSERVICE_H
