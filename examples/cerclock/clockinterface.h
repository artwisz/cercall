/*!
 * \file
 * \brief     Cercall example - Clock service interface
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

#ifndef CERCALL_CLOCKINTERFACE_H
#define CERCALL_CLOCKINTERFACE_H

#include <chrono>
#include "cercall/cercall.h"
#include "../manageableserviceinterface.h"

using ClockAlarmId = uint32_t;

struct ClockEventBase
{
    virtual ~ClockEventBase() {}
    virtual const char* get_class_name() const = 0;

    template<class DerivedEvent>
    const DerivedEvent* get_as() const
    {
        return dynamic_cast<const DerivedEvent*>(this);
    }

    template<class A>
    void serialize(A& ar, unsigned int /*version*/)
    {
        (void)ar;
    }

protected:
    ClockEventBase() {}
};

struct ClockAlarmEvent : public ClockEventBase
{
    ClockAlarmEvent() : ClockEventBase() {}
    ClockAlarmEvent(ClockAlarmId alarm, const std::string &tag) : myAlarmId(alarm), myTag(tag) {}
    const char* get_class_name() const override { return "ClockAlarmEvent"; }

    ClockAlarmId myAlarmId;
    std::string myTag;

    template<class A>
    void serialize(A& ar, unsigned int /*version*/);
};

struct ClockTickEvent : public ClockEventBase
{
    ClockTickEvent() : ClockEventBase() {}
    ClockTickEvent(std::chrono::system_clock::time_point time)
    : ClockEventBase(), myTickTime(time) {}
    const char* get_class_name() const override { return "ClockTickEvent"; }

    std::chrono::system_clock::time_point myTickTime;

    template<class A>
    void serialize(A& ar, unsigned int /*version*/);
};

O_REGISTER_DERIVED_TYPE(ClockInterface, ManagableServiceInterface);

class ClockInterface : public ManagableServiceInterface
{
public:

    template<typename T>
    using Closure = typename cercall::Closure<T>;

    using EventType = ClockEventBase;

    virtual void get_time(Closure<std::chrono::system_clock::time_point> closure) = 0;

    virtual void set_tick_interval(std::chrono::milliseconds tickInterval, Closure<void> closure) = 0;

    virtual void set_alarm(std::string tag, std::chrono::system_clock::duration after, Closure<ClockAlarmId> closure) = 0;

    virtual void cancel_alarm(ClockAlarmId alarm, Closure<void> closure) = 0;
};

#endif //CERCALL_CLOCKINTERFACE_H
