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

#include <pybind11/pybind11.h>
#include <pybind11/chrono.h>
#include "pyclockclient.h"
#include "cercall/asio/clienttcptransport.h"

namespace py = pybind11;

static asio::io_service ioService;
static std::shared_ptr<asio::io_service::work> iosWork;

ClockClient::ClockClient(const std::string& host, const std::string& port)
: Client(std::make_unique<cercall::asio::ClientTcpTransport>(ioService, host, port))
{
    add_listener(myListener);
    
    iosWork = std::make_shared<asio::io_service::work>(ioService);
}

bool ClockClient::open()
{
    return Client::open();
}


void ClockClient::poll__io()
{
    ioService.poll_one();
}

void ClockClient::get__time()
{
    std::cout << "get__time\n";
    get_time ( [this] ( const cercall::Result<std::chrono::system_clock::time_point>& res ) {
        std::cout << "got result for get__time\n";
        if ( !res ) {
            throw std::runtime_error ( res.error().message() );
        } else {
            on_get_time_result ( res.get_value() );
        }
    } );
}

void ClockClient::set__tick_interval ( std::chrono::milliseconds tickInterval )
{
    set_tick_interval ( tickInterval, [] ( const cercall::Result<void>& result ) {
        if ( result.error() ) {
            throw std::runtime_error ( result.error().message() );
        }
    } );
}

void ClockClient::set__alarm ( std::string tag, std::chrono::system_clock::duration after )
{
    set_alarm ( tag, after, [this] ( const cercall::Result<ClockAlarmId>& result ) {
        if ( result.error() ) {
            throw std::runtime_error ( result.error().message() );
        } else {
            on_set_alarm_result ( result.get_value() );
        }
    } );
}

void ClockClient::ClockListener::on_service_event ( std::unique_ptr<ClockClient::EventType> event )
{
    std::cout << "received " << event->get_class_name() << "\n";
    const ClockAlarmEvent* clockEvent = event->get_as<ClockAlarmEvent>();
    if ( clockEvent != nullptr ) {
        myClient.on_alarm_event(clockEvent->myAlarmId, clockEvent->myTag);
    } else {
        const ClockTickEvent* clockTickEvent = event->get_as<ClockTickEvent>();
        if ( clockTickEvent != nullptr ) {
            myClient.on_tick_event(clockTickEvent->myTickTime);
        }
    }
}

class PyClockClient : public ClockClient {
public:
    /* Inherit the constructors */
    using ClockClient::ClockClient;

    /* Trampoline (need one for each virtual function) */
    void on_get_time_result(std::chrono::system_clock::time_point result) override {
        PYBIND11_OVERLOAD_PURE(
            void, ClockClient, on_get_time_result, result
        );
    }
    
    void on_set_alarm_result(ClockAlarmId result) override {
        PYBIND11_OVERLOAD_PURE(
            void, ClockClient, on_set_alarm_result, result
        );
    }
    
    void on_tick_event(std::chrono::system_clock::time_point tickTime) override {
        PYBIND11_OVERLOAD_PURE(
            void, ClockClient, on_tick_event, tickTime
        );
    }
    
    void on_alarm_event(ClockAlarmId alarmId, std::string tag) override {
        PYBIND11_OVERLOAD_PURE(
            void, ClockClient, on_alarm_event, alarmId, tag
        );
    }
    
};

PYBIND11_MODULE(pyclock, m) {
py::class_<ClockClient, PyClockClient>(m, "ClockClient")
    .def(py::init<const std::string&, const std::string&>())
    .def("open", &ClockClient::open)
    .def("poll__io", &ClockClient::poll__io)
    .def("get__time", &ClockClient::get__time)
    .def("set__tick_interval", &ClockClient::set__tick_interval)
    .def("set__alarm", &ClockClient::set__alarm);
}
