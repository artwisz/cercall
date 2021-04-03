/*!
 * \file
 * \brief     Cercall tests - testing errors
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

#include "debug.h"
#include "gtest/gtest.h"
#include "cercall/asio/clienttcptransport.h"
#include "calculatorclient.h"
#include "process.h"
#include "testutil.h"
#include "cercall/asio/tcpacceptor.h"
#include "calculatorservice.h"

class ErrorsTest : public testing::Test, public TestUtil<CalculatorClient<CalculatorInterface::Serialization>, ErrorsTest>
{
public:
    static std::unique_ptr<cercall::Transport> create_client_transport(asio::io_service &ios)
    {
        return cercall::make_unique<cercall::asio::ClientTcpTransport>(ios, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    }

protected:

    static void SetUpTestCase()
    {
        start_service(nullptr);
    }

    static void TearDownTestCase()
    {
        stop_service();
    }

    void SetUp() override
    {
        myClient = create_open_client(myIoService);
    }

    void TearDown() override
    {
        myClient->close();
    }
};

TEST_F(ErrorsTest, test_call_closed_client)
{
    myClient->close();
    EXPECT_THROW(myClient->add(0, 1, 2, [](const cercall::Result<int32_t>&){}), std::runtime_error);
}

TEST_F(ErrorsTest, test_error_from_service)
{
    bool gotResult = false;
    myClient->add(0, 1000, INT32_MAX - 500, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_TRUE( !res);
        EXPECT_EQ(res.error().code(), EOVERFLOW);
        gotResult = true;
    });
    EXPECT_EQ(process_io_events(gotResult, 2), true);
}

TEST_F(ErrorsTest, test_broken_connection)
{
    bool gotResult = false;
    myClient->add_and_delay_result(321, 123, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_TRUE( !res);
        EXPECT_EQ(res.error().code(), ::asio::error::eof);
        //std::cout << "error: " << res.error().code() << " (" << res.error().message() << ")\n";
        gotResult = true;
    });

    myServerProc.shutdown();
    myServerProc.wait();

    EXPECT_EQ(process_io_events(gotResult, 4), true);

    EXPECT_THROW(myClient->add(0, 1, 2, [](const cercall::Result<int32_t>&){ }), std::runtime_error);

    start_service(nullptr);
}

TEST_F(ErrorsTest, test_accept_error)
{
    auto acceptor = cercall::make_unique<cercall::asio::TcpAcceptor>(myIoService, TEST_SERVICE_PORT);
    auto closeAction = [](){};
    using CalculatorServiceType = CalculatorService<CalculatorInterface::Serialization>;
    std::shared_ptr<CalculatorServiceType> service = std::make_shared<CalculatorServiceType>(myIoService, std::move(acceptor),
                                                                                     closeAction);
    EXPECT_THROW(service->start(), std::runtime_error);
}

TEST_F(ErrorsTest, test_double_call_on_service)
{
    //The Client class template protects against a 2nd function call on Service before
    //receving response from the 1st call. So lower-level classes must be used to stimulate that condition.
    using namespace cercall::details;
    using Serialization = CalculatorInterface::Serialization;
    auto transport = std::make_shared<cercall::asio::ClientTcpTransport>(myIoService, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    std::string funcName = "CalculatorInterface::add_and_delay_result";
    int32_t param = 0;
    std::string callMsg = CalculatorInterface::Serialization::serialize_call(nullptr, funcName, param, param);

    struct ClientMock : public cercall::asio::ClientTcpTransport::Listener
    {
        ClientMock() : messenger ([](cercall::Transport&, const std::string& msg) {
            Serialization::deserialize_call(nullptr, msg, [](const std::string& funcName, Serialization::InputArchive& arRes){
                EXPECT_EQ(funcName, "CalculatorInterface::add_and_delay_result");
                cercall::Result<int32_t> result = Serialization::deserialize_result<int32_t>(arRes);
                //std::cout << "result: " << result.error().message() << '\n';
                EXPECT_EQ(result.error().code(), EINPROGRESS);
            });
        })
        {
        }
        bool open(cercall::Transport& t)
        {
            bool res = t.open();
            messenger.init_transport(t);
            return res;
        }
        std::size_t on_incoming_data(cercall::Transport& tr, std::size_t dataLenInBuffer) override
        {
            return messenger.read(tr, dataLenInBuffer);
        }

        void on_connection_error(cercall::Transport&, const cercall::Error&) override
        {
            cercall::log<cercall::debug>(O_LOG_TOKEN, "on_connection_error");
        }
        void on_disconnected(cercall::Transport&) override
        {
        }
        Messenger messenger;

    } clientMock;

    transport->set_listener(clientMock);
    clientMock.open(*transport);
    Messenger::write_message_with_header(*transport, callMsg);
    Messenger::write_message_with_header(*transport, callMsg);
    myIoService.run_one();
    myIoService.run_one();
    transport->close();
    myIoService.run_one();
    transport->clear_listener();
}

TEST_F(ErrorsTest, test_failed_open)
{
    EXPECT_FALSE(myClient->open()); //repeated open
    myClient->close();

    myServerProc.shutdown();
    myServerProc.wait();

    EXPECT_FALSE(myClient->open());

    start_service(nullptr);
}

TEST_F(ErrorsTest, test_connection_reset)
{
    myClient->close();
    myServerProc.shutdown();
    myServerProc.wait();

    cercall::log<cercall::debug>(O_LOG_TOKEN, " ------ Start test ------");

    start_service("-t");
    myClient = create_open_client(myIoService);
    EXPECT_TRUE(myClient->is_open());
    bool gotResult = false;

    myClient->add(1, 2, 3, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_TRUE( !res);
        //std::cout << "result 1: " << res.error().message() << '\n';
        EXPECT_EQ(res.error().code(), ECONNRESET);
        gotResult = true;
    });
    EXPECT_EQ(process_io_events(gotResult, 2), true);

    myClient->close();

    myServerProc.wait();

    start_service("-t");

    myClient = create_open_client(myIoService);
    myServerProc.wait();
    gotResult = false;
    myClient->add(4, 5, 6, [&gotResult](const cercall::Result<int32_t>& res){
        EXPECT_TRUE( !res);
        //std::cout << "result 2: " << res.error().message() << '\n';
        EXPECT_EQ(res.error().code(), ECONNRESET);
        gotResult = true;
    });
    EXPECT_EQ(process_io_events(gotResult, 4), true);

    start_service(nullptr);
}

TEST_F(ErrorsTest, test_resolve_error)
{
    myClient->close();

    std::cout << " ------ Start test ------\n";

    std::string invalidHost = "notexistingandinvalidhostaddress.org";
    auto transport = cercall::make_unique<cercall::asio::ClientTcpTransport>(myIoService, invalidHost, TEST_SERVICE_PORT_STR);
    myClient = std::make_shared<CalculatorClient<CalculatorInterface::Serialization>>(std::move(transport));

    EXPECT_FALSE(myClient->is_open());

    bool gotResult = false;
    myClient->open([&gotResult](const cercall::Result<bool>& res) {
        gotResult = true;
        EXPECT_TRUE( !res);
        //std::cout << "open result: " << res.error().code() << " - " << res.error().message() << '\n';
        EXPECT_EQ(res.error().code(), HOST_NOT_FOUND);
        EXPECT_EQ(res.error().category<cercall::asio::ErrorCategory>(), ::asio::error::get_netdb_category());
    });

    EXPECT_EQ(process_io_events(gotResult, 2), true);
}

TEST_F(ErrorsTest, test_async_connect_error)
{
    myClient->close();
    myServerProc.shutdown();
    myServerProc.wait();

    std::cout << " ------ Start test ------\n";

    auto transport = cercall::make_unique<cercall::asio::ClientTcpTransport>(myIoService, TEST_SERVICE_HOST, TEST_SERVICE_PORT_STR);
    myClient = std::make_shared<CalculatorClient<CalculatorInterface::Serialization>>(std::move(transport));

    EXPECT_FALSE(myClient->is_open());

    bool gotResult = false;
    myClient->open([&gotResult](const cercall::Result<bool>& res) {
        gotResult = true;
        EXPECT_TRUE( !res);
        //std::cout << "open result: " << res.error().code() << " - " << res.error().message() << '\n';
        EXPECT_EQ(res.error().code(), ECONNREFUSED);
        EXPECT_EQ(res.error().category<cercall::asio::ErrorCategory>(), ::asio::error::get_system_category());
    });

    EXPECT_EQ(process_io_events(gotResult, 4), true);

    start_service(nullptr);
}
