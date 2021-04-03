/*!
 * \file
 * \brief     Cercall Service class
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

#ifndef CERCALL_SERVICE_H
#define CERCALL_SERVICE_H

#include <unordered_map>
#include <deque>
#include <vector>
#include <thread>
#include "cercall/transport.h"
#include "cercall/acceptor.h"
#include "cercall/details/functiondict.h"
#include "cercall/details/messenger.h"
#include "cercall/details/cpputil.h"
#include "cercall/details/eventhelper.h"
#include "cercall/log.h"

/**
 * @ingroup Cercall
 * @brief A macro for adding service functions to a service's function dictionary.
 * The macro has to be used in the constructor of the service implementation class.
 *
 * @param Interface the name of the service interface class
 * @param oneway false for functions which receive a closure feedback from the service,
 *               true for one-way functions, which don't have the closure argument
 * @param ... from one to eight Interface member function names, without any prefix, not quoted
 *
 * @note Overloading of interface member functions is currently not supported, so the added
 * member functions must have unique names - each member function must be added once.
 */
#define O_ADD_SERVICE_FUNCTIONS_OF(Interface, oneway, ...)  {                                 \
    std::deque<const char*> funcNames = { CERCALL_APPLY(CERCALL_STRINGIZE, 0, __VA_ARGS__) };       \
    this->template add_functions<oneway>(funcNames, CERCALL_APPLY(CERCALL_MAKE_MEMFUN, Interface, __VA_ARGS__));   \
    }

namespace cercall {

/**
 * @ingroup Cercall
 * @brief A template class for implementing Cercall services.
 */
template<class ServiceInterface, class Serialization = ServiceInterface>
class Service : public ServiceInterface, protected Transport::Listener,
                protected Acceptor::Listener
{
public:
    using InterfaceType = ServiceInterface;

    /** EventType evaluates to void when ServiceInterface does not define it. */
    using EventType = typename details::complement_event_type<ServiceInterface>::type;

    /**
     * @brief Service constructor
     * @param ac - pointer to an acceptor object, the service takes ownership of this acceptor object
     */
    Service(std::unique_ptr<Acceptor> ac) : myAcceptor(std::move(ac))
    {
        o_assert (myAcceptor.get() != nullptr);
#ifdef O_ENSURE_SINGLE_THREAD
        myThreadId = std::this_thread::get_id();
#endif
        myAcceptor->set_listener(*this);
    }
    virtual ~Service()
    {
        stop();
    }

    /**
     * @brief Start accepting client connections to the service.
     * @param maxPendingClientConnections @see Acceptor::open()
     */
    void start(int maxPendingClientConnections = -1)
    {
        check_thread_id("cercall::Service::start");
        if ( !myAcceptor->is_open()) {
            myAcceptor->open(maxPendingClientConnections);
        }
    }

    /**
     * @brief Stop the service closing the server endpoint.
     * All clients connections are closed.
     */
    void stop()
    {
        check_thread_id("cercall::Service::stop");
        if (myAcceptor != nullptr && myAcceptor->is_open()) {
            myAcceptor->close();
            while (myClients.size() > 0) {
                myClients.begin()->second.myTransport->close();
            }
        }
    }

    /**
     * @brief Broadcast a polymorphic event to each connected client.
     * Event types must be serializable.
     * The function constructs an event object passing the function arguments to the constructor.
     * @args arguments to the constructor of the derived event type
     */
    template<typename DerivedET, typename... EventArgs>
    typename std::enable_if<std::is_polymorphic<EventType>{} && std::is_base_of<EventType, DerivedET>{}>::type
    broadcast_event(EventArgs&&... args)
    {
        std::unique_ptr<EventType> e { new DerivedET(std::forward<EventArgs>(args)...) };
        broadcast(e);
    }

    /**
     * @brief Broadcast a non-polymorphic event to each connected client.
     */
    template<typename E = EventType>
    typename std::enable_if<!std::is_polymorphic<E>{}>::type
    broadcast_event(const E& ev)
    {
        static_assert(std::is_same<E, EventType>::value, "Invalid event parameter type");
        broadcast(ev);
    }

    /**
     * @note The shared_ptr may not be used because of the Cereal object tracking feature, which prevents reuse of
     * the binary archive when using shared_ptr.
     */
    template<typename E = EventType>
    void broadcast_event(const std::shared_ptr<EventType>&)
    {
        static_assert( !std::is_same<E, E>::value, "Please do not use shared_ptr type for events");
    }

    template<typename DerivedET>
    typename std::enable_if<std::is_polymorphic<EventType>{} && std::is_base_of<EventType, DerivedET>{}>::type
    broadcast_event(const std::shared_ptr<DerivedET>&)
    {
        static_assert( !std::is_same<DerivedET, DerivedET>::value, "Please do not use shared_ptr type for events");
    }

protected:

    template<bool OneWay, typename HeadFunc, typename ...Funcs>
    void add_functions(std::deque<const char*>& funcNames, HeadFunc hFunc, Funcs... tailFuncs)
    {
        check_thread_id("cercall::Service::add_functions");
        add_function<OneWay>(std::string(funcNames.front()), hFunc);
        funcNames.pop_front();
        add_functions<OneWay>(funcNames, tailFuncs...);
    }

    template<bool OneWay, typename MemF>
    void add_functions(std::deque<const char *> &funcNames, MemF func)
    {
        check_thread_id("cercall::Service::add_functions");
        add_function<OneWay>(std::string(funcNames.front()), func);
    }

    std::vector<std::shared_ptr<Transport>> get_clients()
    {
        decltype(get_clients()) result;
        for (auto& cl: myClients) {
            result.push_back(cl.second.myTransport);
        }
        return result;
    }

private:

    /// @brief The class to be used to deserialize function arguments.
    using ArgsArchive = typename Serialization::InputArchive;
    /// @brief The function dictionary.
    using FunctionDictionary = details::FunctionDict<InterfaceType, Serialization>;

    const std::string bcastFuncName = std::string(details::TypeProperties<InterfaceType>::name) + "::broadcast_event";

    struct Archives
    {
        using S = Serialization;
        std::unique_ptr<typename S::OutputArchive> outArch;
        std::unique_ptr<typename S::InputArchive> inArch;

        template<typename S = Serialization, typename std::enable_if<S::REUSABLE_ARCHIVE>::type* = nullptr>
        Archives() : outArch { Serialization::create_output_archive() },
                     inArch  { Serialization::create_input_archive() } {}

        template<typename S = Serialization, typename std::enable_if< !S::REUSABLE_ARCHIVE>::type* = nullptr>
        Archives() {}
    };

    struct ClientState
    {
        ClientState(const std::shared_ptr<Transport>& t, const details::Messenger& r)
            : myTransport(t), myMessenger(r) {}
        std::shared_ptr<Transport> myTransport { nullptr };
        details::Messenger myMessenger;
        Archives myArchives;

        typename Serialization::OutputArchive* get_output_archive()
        {
            return myArchives.outArch.get();
        }

        typename Serialization::InputArchive* get_input_archive()
        {
            return myArchives.inArch.get();
        }
    };

    FunctionDictionary myFuncDict;

    using PendingCallsMap = std::unordered_multimap<std::string, Transport*>;

    std::unique_ptr<Acceptor> myAcceptor;
    std::map<Transport*, ClientState> myClients;
    PendingCallsMap myPendingCalls;
#ifdef O_ENSURE_SINGLE_THREAD
    std::thread::id myThreadId;
#endif

    void check_thread_id(const std::string& errorMsg)
    {
#ifdef O_ENSURE_SINGLE_THREAD
        if (std::this_thread::get_id() != myThreadId) {
            throw std::logic_error(errorMsg + ": call from a foreign thread not supported");
        }
#else
        (void)errorMsg;
#endif
    }

    std::string make_fully_qualified_name(const std::string& shortFuncName)
    {
        return std::string(details::TypeProperties<InterfaceType>::name) + "::" + shortFuncName;
    }

    template<bool OneWay, typename SI, typename ...Args>
    void add_function(const std::string& functionName, void (SI::*function)(Args...))
    {
        static_assert(std::is_base_of<SI, Service<ServiceInterface, Serialization>>::value,
                      "Invalid 1st argument to O_ADD_SERVICE_FUNCTIONS_OF");
        myFuncDict.template add_function<OneWay>(make_fully_qualified_name(functionName), function);
    }

    void on_client_accepted(std::shared_ptr<Transport> clientTrans) override
    {
        auto messageHandler = [this] (Transport& cl, const std::string& msg) {
            auto found = myClients.find(&cl);
            o_assert(found != myClients.end());
            ClientState& cs = found->second;
            Serialization::deserialize_call(cs.get_input_archive(),  msg,
                                            [this, &cs](const std::string& funcName, ArgsArchive& arArgs) {
                dispatch_func(cs, funcName, arArgs);
            });
        };
        clientTrans->set_listener(*this);
        ClientState cs { clientTrans, details::Messenger(messageHandler) };
        auto retval = myClients.emplace(std::make_pair(clientTrans.get(), std::move(cs)));
        if (retval.second) {
            clientTrans->open();        //start receiving messages
            retval.first->second.myMessenger.init_transport(*clientTrans);
        } else {
            throw std::runtime_error("cercall::Service::on_client_accepted: failed to add client to map");
        }
    }

    void on_accept_error(const Error& e) override
    {
        check_thread_id("cercall::Service::on_accept_error");
        throw std::runtime_error("cercall::Service::on_accept_error: accept failed: " + e.message());
    }

    std::size_t on_incoming_data(Transport& client, std::size_t dataLenInBuffer) override
    {
        check_thread_id("cercall::Service::on_incoming_data");
        auto clStateIt = myClients.find(&client);
        o_assert(clStateIt != myClients.end());
        return clStateIt->second.myMessenger.read(client, dataLenInBuffer);
    }

    /* The default implementation does nothing. Service implementation classes can override it if they need it.  */
    void on_connection_error(Transport&, const Error&) override
    {
    }

    void on_disconnected(Transport& client) override
    {
        check_thread_id("cercall::Service::on_disconnected");
        client.clear_listener();
        myClients.erase(&client);
        for (const auto& call : myPendingCalls) {
            if (call.second == &client && call.first.length() != 0) {
                log<error>(O_LOG_TOKEN, "warning - cercall::Service::on_disconnected: client disconnected"
                                        "while call %s is pending", call.first.c_str());
            }
        }
    }

    template<typename T>
    void broadcast(T&& ev)
    {
        check_thread_id("cercall::Service::broadcast");
        if ( !myClients.empty()) {
            for (auto& client : myClients) {
                ClientState& cs = client.second;
                std::string msg { Serialization::template serialize_event<T>(cs.get_output_archive(), bcastFuncName,
                                                                             std::forward<T>(ev)) };
                details::Messenger::write_message_with_header(*client.second.myTransport, msg);
            }
        }
    }

    PendingCallsMap::const_iterator find_pending_call(const std::string& funcName, const Transport* client)
    {
        auto range = myPendingCalls.equal_range(funcName);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == client) {
                return it;
            }
        }
        return myPendingCalls.end();
    }

    void dispatch_func(ClientState& cs, const std::string& funcName, ArgsArchive& args)
    {
        bool isOneWay = myFuncDict.is_one_way(funcName);
        Transport& client = *cs.myTransport;
        bool isPreviousCallPending;

        if ( !isOneWay) {
            //The same call is pending for this client - not allowed.
            isPreviousCallPending = find_pending_call(funcName, &client) != myPendingCalls.end();
        } else {
            isPreviousCallPending = false;
        }
        if (isPreviousCallPending) {
            const Error& err = Error::operation_in_progress();
            //Return error to the client, previous call is not finished yet.
            Result<void> res(err);
            std::string resMsg = Serialization::template serialize_call_result<void>(cs.get_output_archive(), funcName, res);
            details::Messenger::write_message_with_header(client, resMsg);
            return;
        }
        if ( !isOneWay) {
            myPendingCalls.emplace(funcName, &client);
        }
        try {
            auto clientTr = cs.myTransport;
            if ( !isOneWay) {
                auto resultHandler = [this, funcName, clientTr] (std::string& resultMsg) {
                    send_result(clientTr, funcName, resultMsg);
                };
                myFuncDict.call_function(clientTr, funcName, *this, cs.get_output_archive(), args, resultHandler);
            } else {
                static auto oneWayResultHandler = [](std::string&) {
                    o_assert("cercall::Service::dispatch_func: one way result handler was called" == nullptr);
                };
                myFuncDict.call_function(clientTr, funcName, *this, cs.get_output_archive(), args, oneWayResultHandler);
            }
        } catch (const std::exception& e) {
            std::string msg = "failed to call cercall function " + funcName + ": " + e.what();
            throw std::runtime_error(std::string("cercall::Service::dispatch_func: ") + msg.c_str());
        }
    }

    void send_result(const std::shared_ptr<Transport>& cl, const std::string& funcName, std::string& resultMsg)
    {
#ifdef O_ENSURE_SINGLE_THREAD
        o_assert(std::this_thread::get_id() == myThreadId);
#endif
        o_assert(cl.get() != nullptr);
        PendingCallsMap::const_iterator foundPendingCall;

        foundPendingCall = find_pending_call(funcName, cl.get());
        if (foundPendingCall == myPendingCalls.end()) {
            std::string err = std::string("method results already delivered for ") + funcName;
            throw std::runtime_error(std::string("cercall::Service::send_result: ") + err.c_str());
        }

        if (myClients.find(foundPendingCall->second) != myClients.end()) {
            details::Messenger::write_message_with_header(*foundPendingCall->second, resultMsg);
        } else {
            //warning - client disconnected
            log<error>(O_LOG_TOKEN, "warning - cercall::Service::send_result: can't send result of %s "
                                    "to disconnected client", funcName.c_str());
        }
        myPendingCalls.erase(foundPendingCall);
    }

};

} //namespace cercall

#endif // CERCALL_SERVICE_H
