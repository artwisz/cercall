/*!
 * \file
 * \brief     Cercall example - Clock client implementation
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

#include "clockclient.h"
#include "cercall/asio/clienttcptransport.h"
#include <iomanip>

static asio::io_service ioService;
static std::shared_ptr<asio::io_service::work> iosWork;
static ClockAlarmId stopAlarm = 0;

static void finish(void)
{
    std::cout << "finish client program now\n";
    std::cout.flush();
    iosWork.reset();
    ioService.stop();
}

std::string time_point_to_str(const std::chrono::system_clock::time_point& tp)
{
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time);
    std::stringstream ss;
    ss << std::put_time(&tm, "%T");
    return ss.str();
}

struct ClockListener : public ClockClient::ServiceListener
{
    ClockListener(std::shared_ptr<ClockClient>& cc) : myClient(cc) {}
    void on_service_event(std::unique_ptr<ClockClient::EventType> event) override
    {
        std::cout << "received " << event->get_class_name() << "\n";
        const ClockAlarmEvent* clockEvent = event->get_as<ClockAlarmEvent>();
        if (clockEvent != nullptr) {
            if (clockEvent->myAlarmId == stopAlarm) {
                myClient->close_service("client closed", [](const cercall::Result<int>& result) {
                    if (!result) {
                        std::cerr << "close_service error: " << result.error().message() << '\n';
                    }
                    if (result.get_value() != 0) {
                        std::cerr << "close_service result: " << result.get_value() << '\n';
                    }
                    finish();
                });
            }
        } else {
            const ClockTickEvent* clockTickEvent = event->get_as<ClockTickEvent>();
            if (clockTickEvent != nullptr) {
                std::cout << "tick time: " << time_point_to_str(clockTickEvent->myTickTime) << std::endl;
            }
        }
    }
private:
    std::shared_ptr<ClockClient> myClient;
};

void get_time(std::shared_ptr<ClockClient>& cc)
{
    cc->get_time([](const cercall::Result<std::chrono::system_clock::time_point>& res){
        if ( !res) {
            std::cerr << "get_time failed\n";
            throw std::runtime_error(res.error().message());
        } else {
            std::cout << "Current time: " << time_point_to_str(res.get_value()) << std::endl;
        }
    });
}

void set_stop_alarm(std::shared_ptr<ClockClient>& cc, std::chrono::system_clock::duration after)
{
        cc->set_alarm("stopClient", after, [after](const cercall::Result<ClockAlarmId>& result){
            if (result.error()) {
                std::cerr << "set_alarm failed: " << result.error().message() << '\n';
                throw std::runtime_error(result.error().message());
            } else {
                std::cout << "stop alarm set in "
                          << std::chrono::duration_cast<std::chrono::seconds>(after).count() << " sec.\n";
                stopAlarm = result.get_value();
            }
        });
}

void set_clock_tick(asio::io_service& ios, std::shared_ptr<ClockClient>& cc, std::chrono::milliseconds interval)
{
    ios.post([cc, interval]() {
        cc->set_tick_interval(interval, [](const cercall::Result<void>& result){
            if ( result.error() ) {
                throw std::runtime_error ( result.error().message() );
            }
        });
    });
}


int main(int ac, char **av)
{
    (void)ac;
    (void)av;

    try {
        auto clientTransport = std::make_unique<cercall::asio::ClientTcpTransport>(ioService, "127.0.0.1", "4321");
        auto client = std::make_shared<ClockClient>(std::move(clientTransport));

        ClockListener clockListener(client);

        client->add_listener(clockListener);

        if( !client->open()) {
            std::cerr << "Could not connect to clock server\n";
            return 1;
        }

        iosWork = std::make_shared<asio::io_service::work>(ioService);

        get_time(client);

        set_stop_alarm(client, std::chrono::seconds(10));

        set_clock_tick(ioService, client, std::chrono::milliseconds(2000));

        ioService.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    ioService.stop();
    return 0;
}
