/*!
 * \file
 * \brief     PolyEventSource service interface for Cercall tests
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

#ifndef CERCALL_POLYEVENTSOURCEINTERFACE_H
#define CERCALL_POLYEVENTSOURCEINTERFACE_H

#include "cercall/cercall.h"
#if defined(TEST_CEREAL_BINARY)
#include "cercall/cereal/binary.h"
#elif defined(TEST_CEREAL_JSON)
#include "cercall/cereal/json.h"
#elif defined(TEST_BOOST_TEXT)
#include "cercall/boost/text.h"
#elif defined(TEST_BOOST_BINARY)
#include "cercall/boost/binary.h"
#endif

#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/map.hpp>
#define USING_CEREAL
#elif defined(TEST_BOOST_TEXT) || defined(TEST_BOOST_BINARY)
#include <boost/serialization/access.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/map.hpp>
#define USING_BOOST
#endif

struct EventBase
{
    using Ptr = std::unique_ptr<EventBase>;

    EventBase() {}
    virtual ~EventBase() {}

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
};

struct RealEventClassOne : public EventBase
{
    RealEventClassOne() : EventBase() {}
    RealEventClassOne(const std::string &eventData) : myEventData(eventData) {}

    std::string myEventData;

    template<class A>
    void serialize(A& ar, unsigned int /*version*/ )
    {
#ifdef USING_CEREAL
        ar(cereal::base_class<EventBase>(this), myEventData);
#elif defined(USING_BOOST)
        ar & boost::serialization::base_object<EventBase>(*this)  & myEventData;
#endif
    }
};

#ifdef USING_CEREAL
CEREAL_REGISTER_TYPE(RealEventClassOne);
#elif defined(USING_BOOST)
BOOST_CLASS_EXPORT(RealEventClassOne);
#endif

struct RealEventClassTwo : public EventBase
{
    RealEventClassTwo() : EventBase() {}
    RealEventClassTwo(int eventData) : myEventData(eventData) {}

    int myEventData;

    template<class A>
    void serialize(A& ar, unsigned int /*version*/)
    {
#ifdef USING_CEREAL
        ar(cereal::base_class<EventBase>(this), myEventData);
#elif defined(USING_BOOST)
        ar & boost::serialization::base_object<EventBase>(*this) & myEventData;
#endif
    }
};

#ifdef USING_CEREAL
CEREAL_REGISTER_TYPE(RealEventClassTwo);
#elif defined(USING_BOOST)
BOOST_CLASS_EXPORT(RealEventClassTwo)
#endif

struct RealEventClassThree : public RealEventClassTwo
{
    using EventDataType = std::map<std::string, int>;
    RealEventClassThree() : RealEventClassTwo() {}
    RealEventClassThree(const EventDataType &eventDict) : myEventDict(eventDict) {}

    EventDataType myEventDict;

    template<class A>
    void serialize(A& ar, unsigned int /*version*/)
    {
#ifdef USING_CEREAL
        ar(cereal::base_class<RealEventClassTwo>(this), myEventDict);
#elif defined(USING_BOOST)
        ar & boost::serialization::base_object<RealEventClassTwo>(*this) & myEventDict;
#endif
    }
};

#ifdef USING_CEREAL
CEREAL_REGISTER_TYPE(RealEventClassThree);
#elif defined(USING_BOOST)
BOOST_CLASS_EXPORT(RealEventClassThree)
#endif

O_REGISTER_TYPE(PolyEventSourceInterface);

class PolyEventSourceInterface
#if defined(TEST_CEREAL_BINARY)
    : public cercall::cereal::Binary
#elif defined(TEST_CEREAL_JSON)
    : public cercall::cereal::Json
#elif defined(TEST_BOOST_TEXT)
    : public cercall::boost::Text
#elif defined(TEST_BOOST_BINARY)
    : public cercall::boost::Binary
#endif
{
public:

    template<typename T>
    using Closure = typename cercall::Closure<T>;

    using EventType = EventBase;

#ifdef USING_CEREAL
    static_assert(cereal::traits::is_input_serializable<EventType, InputArchive>::value == true, "");
#endif

    virtual void trigger_single_broadcast(std::unique_ptr<EventType> e) = 0;
};

#endif // CERCALL_POLYEVENTSOURCEINTERFACE_H
