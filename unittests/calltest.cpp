/*!
 * \file
 * \brief     Cercall tests - testing function calls
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

#include "gtest/gtest.h"
#include "utils/debug.h"
#include "calculatorinterface.h"
#include "cercall/client.h"
#include "cercall/asio/clienttcptransport.h"
#include "calculatorclient.h"
#include "process.h"
#include "testutil.h"
#include <random>

using cercall::test::Process;

extern std::vector<const char*> programArguments;

class CallTest : public testing::Test, public TestUtil<CalculatorClient<CalculatorInterface::Serialization>, CallTest>
{
public:
    static std::unique_ptr<cercall::Transport> create_client_transport(asio::io_service &ios)
    {
        return cercall::make_unique<cercall::asio::ClientTcpTransport>(ios, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    }

protected:

    static void SetUpTestCase()
    {
        for(auto arg: programArguments) {
            if (std::string(arg) == "--test_many_clients") {
                multipleClientTestSlave = true;
                break;
            }
        }
        if ( !multipleClientTestSlave) {
            start_service(nullptr);
        }
    }

    static void TearDownTestCase()
    {
        if ( !multipleClientTestSlave) {
            stop_service();
        }
    }

    void SetUp() override
    {
        myClient = create_open_client(myIoService);
    }

    void TearDown() override
    {
        myClient->close();
    }

    static bool multipleClientTestSlave;
};

bool CallTest::multipleClientTestSlave = false;

TEST_F(CallTest, test_simple_call)
{
    bool gotResult = false;
    myClient->add(12, 23, 34, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (12 + 23 + 34));
        gotResult = true;
    });
    EXPECT_TRUE(myClient->is_call_in_progress("add"));
    EXPECT_EQ(process_io_events(gotResult, 2), true);
    EXPECT_FALSE(myClient->is_call_in_progress("add"));
}

//Boost.Serialization does not support serialization of pointers to primitive types.
#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
TEST_F(CallTest, test_pointers)
{
    bool gotResult = false;
    auto x = std::make_unique<int32_t>(1234);
    auto y = std::make_unique<int32_t>(4321);
    myClient->add_by_pointers(std::move(x), std::move(y), [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (1234 + 4321));
        gotResult = true;
    });
    EXPECT_EQ(process_io_events(gotResult, 2), true);
}
#endif

TEST_F(CallTest, test_queued_calls)
{
    bool gotResultCall1 = false;
    bool gotResultCall2 = false;
    bool gotResultCall3 = false;

    myClient->add(1, 2, 3, [&gotResultCall1](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (1 + 2 + 3));
        gotResultCall1 = true;
    });

   myClient->add(4, 5, 6, [&gotResultCall2](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (4 + 5 + 6));
        gotResultCall2 = true;
    });

   myClient->add(7, 8, 9, [&gotResultCall3](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (7 + 8 + 9));
        gotResultCall3 = true;
    });

   EXPECT_THROW(myClient->add(0, 1, 2, [](const cercall::Result<int32_t>&){}), std::runtime_error);

   EXPECT_EQ(process_io_events(gotResultCall1, 2), true);

   EXPECT_NO_THROW(myClient->add(0, 1, 2, [](const cercall::Result<int32_t>&){}));

   EXPECT_EQ(process_io_events(gotResultCall2, 4), true);
   EXPECT_EQ(process_io_events(gotResultCall3, 4), true);
}

static std::vector<int32_t> generate_data(size_t size)
{
    static std::uniform_int_distribution<int32_t> distribution(
        std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int32_t>::max());
    static std::default_random_engine generator;

    std::vector<int32_t> data(size);
    std::generate(data.begin(), data.end(), []() { return distribution(generator); });
    return data;
}

TEST_F(CallTest, test_large_message)
{
    bool gotResult = false;
    constexpr size_t vectorLength = 1024U;

    std::vector<int32_t> a = generate_data(vectorLength);
    std::vector<int32_t> b = generate_data(vectorLength);
    myClient->add_vector(a, b, [&a, &b, &gotResult](const cercall::Result<std::vector<int64_t>> &res){
        EXPECT_FALSE( !res);
        std::vector<int64_t> localResult(a.size());
        std::transform (a.begin(), a.end(), b.begin(), localResult.begin(), std::plus<int64_t>());
        EXPECT_EQ(res.get_value(), localResult);
        gotResult = true;
    });
    EXPECT_EQ(process_io_events(gotResult, 9), true);
}

TEST_F(CallTest, test_async_open)
{
    myClient->close();

    auto transport = cercall::make_unique<cercall::asio::ClientTcpTransport>(myIoService, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    myClient = std::make_shared<CalculatorClient<CalculatorInterface::Serialization>>(std::move(transport));

    EXPECT_FALSE(myClient->is_open());

    bool gotResult = false;
    myClient->open([&gotResult](const cercall::Result<bool>& res) {
        gotResult = true;
        EXPECT_FALSE( !res);
        //std::cout << "open result: " << res.error().code() << " - " << res.error().message() << '\n';
    });

    EXPECT_EQ(process_io_events(gotResult, 4), true);

    EXPECT_TRUE(myClient->is_open());

    gotResult = false;
    myClient->add(100, 200, 300, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_FALSE( !res);
        ASSERT_EQ(res.get_value(), (100 + 200 + 300));
        gotResult = true;
    });

    EXPECT_EQ(process_io_events(gotResult, 2), true);

    gotResult = false;
    myClient->open([&gotResult](const cercall::Result<bool>& res) {
        gotResult = true;
        EXPECT_TRUE( !res);
        EXPECT_EQ(res.error().code(), EISCONN);
        //std::cout << "open result: " << res.error().code() << " - " << res.error().message() << '\n';
    });

}

TEST_F(CallTest, test_many_clients)
{
    const unsigned numClients = 4u;

    if ( !multipleClientTestSlave) {
        for (unsigned i = 0; i < numClients; ++i) {
            extern Program program;

            std::string cmd = std::string(Program::instance().get_path());
            myServerProc.create(cmd.c_str(), "--gtest_filter=*test_many_clients --test_many_clients");
        }
        unsigned keepRunning = 4u;
        do {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            --keepRunning;
            bool gotResult = false;
            myClient->get_connected_clients_count([&keepRunning, &gotResult](const cercall::Result<size_t>& res){
                EXPECT_FALSE( !res);
                if (res.get_value() == 1u) {    //only the master client is connected
                    keepRunning = 0u;
                }
                gotResult = true;
            });
            EXPECT_EQ(process_io_events(gotResult, 4), true);
        } while(keepRunning > 0u);
    } else {
        auto tStart = std::chrono::system_clock::now();
        size_t callCount = 0u;
        do {
            std::vector<int32_t> v = generate_data(3u);
            bool gotResult = false;
            myClient->add(static_cast<int8_t>(v[0]), static_cast<int16_t>(v[1]),v[2], [&gotResult, &v](const cercall::Result<int32_t>& res){
                EXPECT_FALSE( !res);
                EXPECT_EQ(res.get_value(), (static_cast<int8_t>(v[0]) + static_cast<int16_t>(v[1]) + v[2]));
                gotResult = true;
            });
            EXPECT_EQ(process_io_events(gotResult, 10), true);
            ++callCount;
        } while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - tStart).count() < 2u);
        cercall::log<cercall::debug>(O_LOG_TOKEN, "performed %d calls to add function", callCount);
    }
}
