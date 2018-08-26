/*!
 * \file
 * \brief     Cercall tests utility class
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

#ifndef CERCALL_TESTUTIL_H
#define CERCALL_TESTUTIL_H

#include "process.h"
#include <thread>
#include "cercall/details/cpputil.h"
#include "cercall/asio/clienttcptransport.h"
#include "program.h"

#define TEST_SERVICE_HOST "127.0.0.1"
#define TEST_SERVICE_PORT  56789
#define TEST_SERVICE_PORT_STR "56789"

template<typename T>
struct has_close_method
{
private:
    template<typename U> static auto test(int) -> decltype(std::declval<U>().close_service(), std::true_type());
    template<typename> static std::false_type test(...);

public:
    static constexpr bool value = std::is_same<decltype(test<T>(0)),std::true_type>::value;
};

/**
 * @brief An utility mix-in class for tests.
 */
template<class Client, class Test>
class TestUtil
{
protected:
    TestUtil() {}
    TestUtil (const TestUtil& other) = delete;
    virtual ~TestUtil() {}
    TestUtil& operator= (const TestUtil& other) = delete;

    static void start_service(const char* args)
    {
        extern Program program;

        std::string cmd = std::string(Program::instance().get_path()) + "_server";
        myServerProc.create(cmd.c_str(), args);
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while( !myServerProc.is_running());
    }

    template<typename T = typename Client::InterfaceType>
    static typename std::enable_if<has_close_method<T>::value>::type
    stop_service()
    {
        asio::io_service ios;
        auto client = create_open_client(ios);
        client->close_service();

        myServerProc.wait();
    }

    template<typename T = typename Client::InterfaceType>
    static typename std::enable_if<!has_close_method<T>::value>::type
    stop_service()
    {
        myServerProc.shutdown();
        myServerProc.wait();
    }

    static std::shared_ptr<Client> create_open_client(asio::io_service &ios)
    {
        try {
            int retry = 4;
            while(retry-- > 0) {
                auto clientTransport = Test::create_client_transport(ios);
                auto client = std::make_shared<Client>(std::move(clientTransport));

                if(client->open()) {
                    return client;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << '\n';
            throw;
        }
        throw std::runtime_error("Could not connect to test server");
    }

    bool process_io_events(bool& closureCalled, int maxNumHandlers)
    {
        while ( !closureCalled && maxNumHandlers) {
            myIoService.run_one();
            --maxNumHandlers;
        }
        return closureCalled;
    }

    asio::io_service myIoService;
    std::shared_ptr<Client> myClient;

    static cercall::test::Process myServerProc;

};

template<class Client, class Test>
cercall::test::Process TestUtil<Client, Test>::myServerProc;

#endif // CERCALL_TESTUTIL_H
