/*!
 * \file
 * \brief     Cercall example - Boost setup for Clock service
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

#ifndef CERCALL_CERCLOCK_BOOST_SETUP_H
#define CERCALL_CERCLOCK_BOOST_SETUP_H

#include "cercall/boost/binary.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>
#include "boost_chrono.hpp"

using BinarySerialization = cercall::boost::Binary;

BOOST_CLASS_EXPORT(ClockAlarmEvent);

BOOST_CLASS_EXPORT(ClockTickEvent);

template<class A>
void ClockAlarmEvent::serialize(A& ar, unsigned int /*version*/)
{
    ar & boost::serialization::base_object<ClockEventBase>(*this) & myTag & myAlarmId;
}

template<class A>
void ClockTickEvent::serialize(A& ar, unsigned int /*version*/)
{
    ar & boost::serialization::base_object<ClockEventBase>(*this) & myTickTime;
}

#endif //CERCALL_CERCLOCK_BOOST_SETUP_H
