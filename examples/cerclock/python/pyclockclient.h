/*!
 * \file
 * \brief     Cercall example - Clock client class as Python module
 * 
 *  Copyright (c) 2020, Arthur Wisz
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

#ifndef CERCALL_PYCLOCKCLIENT_H
#define CERCALL_PYCLOCKCLIENT_H

#include "../clockinterface.h"
#include "cercall/client.h"
#ifdef HAS_CEREAL
#include "../cereal_setup.h"
#else
#include "../boost_setup.h"
#endif

class ClockClient : public cercall::Client<ClockInterface, BinarySerialization>
{
public:

    ClockClient(const std::string& host, const std::string& port);
    
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
    
    bool open();
    
    void poll__io();
    
    void get__time(); 
        
    void set__tick_interval(std::chrono::milliseconds tickInterval);
    
    void set__alarm(std::string tag, std::chrono::system_clock::duration after);
    
    virtual void on_get_time_result(std::chrono::system_clock::time_point result) = 0;
    
    virtual void on_set_alarm_result(ClockAlarmId result) = 0;
    
    virtual void on_tick_event(std::chrono::system_clock::time_point tickTime) = 0;
    
    virtual void on_alarm_event(ClockAlarmId myAlarmId, std::string myTag) = 0;

private:
    class ClockListener : public ClockClient::ServiceListener
    {
    public:
        ClockListener(ClockClient& cc) : myClient(cc) {}
        void on_service_event(std::unique_ptr<ClockClient::EventType> event) override;
    private:
        ClockClient& myClient;       
    };
    
    ClockListener myListener { *this };
};

#endif // CERCALL_PYCLOCKCLIENT_H
