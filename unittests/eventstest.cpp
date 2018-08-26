/*!
 * \file
 * \brief     Cercall tests - testing service events
 *
 * Copyright 2018, Arthur Wisz
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

#include <gtest/gtest.h>
#include "cercall/client.h"
#include "cercall/asio/clienttcptransport.h"
#include "process.h"
#include "testutil.h"
#include "simpleeventsourceinterface.h"
#include "polyeventsourceinterface.h"

using cercall::test::Process;

class SimpleEventSourceClient : public cercall::Client<SimpleEventSourceInterface, SimpleEventSourceInterface::Serialization>
{
    using MyBase = cercall::Client<SimpleEventSourceInterface, SimpleEventSourceInterface::Serialization>;
public:
    SimpleEventSourceClient(std::unique_ptr<cercall::Transport> tr) : MyBase(std::move(tr)) {}

    void trigger_single_broadcast(EventType et) override
    {
        send_call(__func__, et);
    }
};

//Can't templetize it because there is no SFINAE of virtual functions.
class PolyEventSourceClient : public cercall::Client<PolyEventSourceInterface>
{
public:
    PolyEventSourceClient(std::unique_ptr<cercall::Transport> tr) : cercall::Client<PolyEventSourceInterface>(std::move(tr)) {}

    void trigger_single_broadcast(const EventType::Ptr& e) override
    {
        send_call(__func__, e);
    }
};

template<typename EventSourceClient>
class EventsTest : public testing::Test, public TestUtil<EventSourceClient, EventsTest<EventSourceClient>>
{
public:
    static std::unique_ptr<cercall::Transport> create_client_transport(asio::io_service &ios)
    {
        return cercall::make_unique<cercall::asio::ClientTcpTransport>(ios, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    }

protected:

    using MyBase = TestUtil<EventSourceClient, EventsTest<EventSourceClient>>;

    static void SetUpTestCase()
    {
        const char *arg;
        if (std::is_same<EventSourceClient, SimpleEventSourceClient>::value) {
            arg = "-s";
        } else if (std::is_same<EventSourceClient, PolyEventSourceClient>::value) {
            arg = "-p";
        }
        MyBase::start_service(arg);
    }

    static void TearDownTestCase()
    {
        MyBase::stop_service();
    }

    void SetUp() override
    {
        MyBase::myClient = MyBase::create_open_client(MyBase::myIoService);
    }

    void TearDown() override
    {
        MyBase::myClient->close();
    }
};

using SimpleEventsTest = EventsTest<SimpleEventSourceClient>;

class SimpleEventsListener : public SimpleEventSourceClient::ServiceListener
{
public:
    SimpleEventsListener()
    {
        reset();
    }
    void on_service_event(const SimpleEventSourceClient::EventType& ev) override
    {
        receivedEvent = ev;
        gotEvent = true;
    }

    void reset()
    {
        gotEvent = false;
        receivedEvent = SimpleEventSourceClient::EventType::NO_EVENT;
    }

    SimpleEventSourceClient::EventType receivedEvent;
    bool gotEvent;
} simpleEventsListener;

TEST_F(SimpleEventsTest, test_simple_events)
{
    myClient->add_listener(simpleEventsListener);
    myClient->trigger_single_broadcast(SimpleEventSourceClient::EventType::EVENT_ONE);
    EXPECT_EQ(process_io_events(simpleEventsListener.gotEvent, 2), true);
    EXPECT_EQ(simpleEventsListener.receivedEvent, SimpleEventSourceClient::EventType::EVENT_ONE);

    simpleEventsListener.reset();
    myClient->trigger_single_broadcast(SimpleEventSourceClient::EventType::EVENT_ONE);
    EXPECT_EQ(process_io_events(simpleEventsListener.gotEvent, 4), true);
    EXPECT_EQ(simpleEventsListener.receivedEvent, SimpleEventSourceClient::EventType::EVENT_ONE);

    simpleEventsListener.reset();
    myClient->trigger_single_broadcast(SimpleEventSourceClient::EventType::EVENT_TWO);
    EXPECT_EQ(process_io_events(simpleEventsListener.gotEvent, 4), true);
    EXPECT_EQ(simpleEventsListener.receivedEvent, SimpleEventSourceClient::EventType::EVENT_TWO);
}

using PolyEventsTest = EventsTest<PolyEventSourceClient>;

class PolyEventsListener : public PolyEventSourceClient::ServiceListener
{
public:
    PolyEventsListener()
    {
        reset();
    }
    void on_service_event(PolyEventSourceClient::EventType::Ptr &ev) override
    {
        receivedEvent = ev;
        gotEvent = true;
    }

    void reset()
    {
        gotEvent = false;
        receivedEvent.reset();
    }

    PolyEventSourceClient::EventType::Ptr receivedEvent;
    bool gotEvent;
} polyEventsListener;

TEST_F(PolyEventsTest, test_poly_events)
{
    {
        myClient->add_listener(polyEventsListener);
        std::string testEventDataClassOne = "test event class one";
        PolyEventSourceClient::EventType::Ptr ev = std::make_shared<RealEventClassOne>(testEventDataClassOne);
        myClient->trigger_single_broadcast(ev);
        EXPECT_EQ(process_io_events(polyEventsListener.gotEvent, 2), true);
        const auto evClassOne = polyEventsListener.receivedEvent->get_as<RealEventClassOne>();
        EXPECT_NE(evClassOne, nullptr);
        EXPECT_EQ(evClassOne->myEventData, testEventDataClassOne);
    }

    {
        polyEventsListener.reset();
        int testEventDataClassTwo = 123654;
        PolyEventSourceClient::EventType::Ptr ev = std::make_shared<RealEventClassTwo>(testEventDataClassTwo);
        myClient->trigger_single_broadcast(ev);
        EXPECT_EQ(process_io_events(polyEventsListener.gotEvent, 2), true);
        const auto evClassTwo = polyEventsListener.receivedEvent->get_as<RealEventClassTwo>();
        EXPECT_NE(evClassTwo, nullptr);
        EXPECT_EQ(evClassTwo->myEventData, testEventDataClassTwo);
    }

    {
        polyEventsListener.reset();
        RealEventClassThree::EventDataType testEventDataClassThree;
        testEventDataClassThree["one"] = 1;
        testEventDataClassThree["two"] = 2;
        testEventDataClassThree["three"] = 3;
        PolyEventSourceClient::EventType::Ptr ev = std::make_shared<RealEventClassThree>(testEventDataClassThree);
        myClient->trigger_single_broadcast(ev);
        EXPECT_EQ(process_io_events(polyEventsListener.gotEvent, 2), true);
        const auto evClassThree = polyEventsListener.receivedEvent->get_as<RealEventClassThree>();
        EXPECT_NE(evClassThree, nullptr);
        EXPECT_EQ(evClassThree->myEventDict, testEventDataClassThree);
    }
}
