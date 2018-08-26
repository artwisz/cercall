/*!
 * \file
 * \brief     SimpleEventSource service interface for Cercall tests
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

#ifndef CERCALL_SIMPLEEVENTSOURCEINTERFACE_H
#define CERCALL_SIMPLEEVENTSOURCEINTERFACE_H

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
#define USING_CEREAL
#elif defined(TEST_BOOST_TEXT) || defined(TEST_BOOST_BINARY)
#include <boost/serialization/access.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/shared_ptr.hpp>
#define USING_BOOST
#endif

O_REGISTER_TYPE(SimpleEventSourceInterface);

class SimpleEventSourceInterface
{
public:

#if defined(TEST_CEREAL_BINARY)
    using Serialization = cercall::cereal::Binary;
#elif defined(TEST_CEREAL_JSON)
    using Serialization = cercall::cereal::Json;
#elif defined(TEST_BOOST_TEXT)
    using Serialization = cercall::boost::Text;
#elif defined(TEST_BOOST_BINARY)
    using Serialization = cercall::boost::Binary;
#endif

    enum class EventType : int32_t
    {
        NO_EVENT, EVENT_ONE, EVENT_TWO, EVENT_THREE
    };

#ifdef USING_CEREAL
    static_assert(cereal::traits::is_input_serializable<EventType, Serialization::InputArchive>::value == true, "");
#endif

    virtual void trigger_single_broadcast(EventType et) = 0;
};

#endif // CERCALL_SIMPLEEVENTSOURCEINTERFACE_H
