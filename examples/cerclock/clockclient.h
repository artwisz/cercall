/*!
 * \file
 * \brief     Cercall example - Clock client class
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

#ifndef CERCALL_CLOCKCLIENT_H
#define CERCALL_CLOCKCLIENT_H

#include "clockinterface.h"
#include "cercall/client.h"
#ifdef HAS_CEREAL
#include "cereal_setup.h"
#else
#include "boost_setup.h"
#endif

class ClockClient : public cercall::Client<ClockInterface, BinarySerialization>
{
public:
    ClockClient(std::unique_ptr<cercall::Transport> tr)
            : cercall::Client<ClockInterface, BinarySerialization>(std::move(tr)) {}

    void get_time(Closure<std::chrono::system_clock::time_point> closure) override
    {
        send_call(__func__, closure);
    }

    void set_tick_interval(std::chrono::milliseconds tickInterval, Closure<void> closure) override
    {
        send_call(__func__, closure, tickInterval);
    }

    void set_alarm(std::string tag, std::chrono::system_clock::duration after, Closure<ClockAlarmId> closure) override
    {
        send_call(__func__, closure, tag, after);
    }

    void cancel_alarm(ClockAlarmId alarm, Closure<void> closure) override
    {
        send_call(__func__, closure, alarm);
    }

    void close_service(const std::string& reason, cercall::Closure<int> closure) override
    {
        send_call(__func__, closure, reason);
    }
};

#endif // CERCALL_CLOCKCLIENT_H
