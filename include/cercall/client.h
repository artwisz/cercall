/*!
 * \file
 * \brief     Cercall Client class
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


#ifndef CERCALL_CLIENT_H
#define CERCALL_CLIENT_H

#include <list>
#include <unordered_map>
#include <thread>
#include "cercall/transport.h"
#include "cercall/details/typeprops.h"
#include "cercall/details/messenger.h"
#include "cercall/details/callqueue.h"
#include "cercall/details/eventhelper.h"
#include "cercall/log.h"

namespace cercall {
namespace client {

/**
 * A Service listener class template for non-polymorphic event types.
 */
template<typename EventType, typename Enable = void>
class ServiceListener
{
public:
    virtual void on_service_event(const EventType&) = 0;
    virtual ~ServiceListener() {}
};

/**
 * A Service listener class template for polymorphic event types.
 */
template<typename EventType>
class ServiceListener<EventType, typename std::enable_if<std::is_polymorphic<EventType>::value>::type>
{
public:
    virtual void on_service_event(std::unique_ptr<EventType> e) = 0;
    virtual ~ServiceListener() {}
};

}   //namespace client

template<class ServiceInterface, class Serialization = ServiceInterface, unsigned MaxCallsInProgress = 1>
class Client : public ServiceInterface, protected Transport::Listener
{
    static_assert(MaxCallsInProgress > 0, "invalid MaxCallsInProgress");
public:

    using InterfaceType = ServiceInterface;

    /** EventType evaluates to void when ServiceInterface does not define it. */
    using EventType = typename details::complement_event_type<ServiceInterface>::type;

    using ServiceListener = client::ServiceListener<EventType>;

    /** \brief Add a listener of Service events */
    void add_listener(ServiceListener& l)
    {
        myEventListeners.push_back(&l);
    }

    /** \brief Remove a listener of Service events */
    void remove_listener(ServiceListener& l)
    {
        myEventListeners.remove(&l);
    }

    /** \brief The constructor of a Client object.
      * \param t a pointer to the transport for the client connection,
      *        the client assumes ownership of the transport object
      */
    Client(std::unique_ptr<Transport> t) : myTransport (std::move(t)), myMessenger (make_message_handler())
    {
    #ifdef O_ENSURE_SINGLE_THREAD
        myThreadId = std::this_thread::get_id();
    #endif
        o_assert(myTransport != nullptr);
        myTransport->set_listener(*this);
    }

    virtual ~Client()
    {
        o_assert(myTransport != nullptr);
        myTransport->close();
        myTransport->clear_listener();
    }

    /** Copy constructor is not permitted. */
    Client(const Client& o) = delete;

    /** Assignement is not permitted. */
    void operator= (const Client& o) = delete;

    /** \brief Open the connection to the service.
      * The client connection has to be opened before sending any function calls to a Cercall service.
      * This function blocks until the open operation is finished.
      * \return true when transport openened successfully, false otherwise.
      */
    bool open()
    {
        check_thread_id("cercall::Client::open()");
        o_assert(myTransport != nullptr);
        return myTransport->open();
    }

    /** \brief Open the connection to the service.
      * The client connection has to be opened before sending any function calls to a Cercall service.
      * This function starts an asynchronous open operation and returns immediately.
      * \see Transport::open for details.
      */
    void open(const cercall::Closure<bool>& cl)
    {
        check_thread_id("cercall::Client::open()");
        o_assert(myTransport != nullptr);
        myTransport->open(cl);
    }

    /** \brief Close the connection to the service.
     *
     */
    void close()
    {
        check_thread_id("cercall::Client::close()");
        o_assert(myTransport != nullptr);
        myTransport->close();
    }

    /** \brief Check if this client is connected to a service. */
    bool is_open()
    {
        o_assert(myTransport != nullptr);
        return myTransport->is_open();
    }

    /** \brief Check if a function call is in progress, awaiting response from the service.
     * \param functionName - the name of the function from the service interface
     * \return true if the function was called and a closure for it is pending.
     */
    bool is_call_in_progress(const char* functionName)
    {
        check_thread_id("cercall::Client::is_call_in_progress()");
        std::string fullfunctionName = myFuncPrefix;
        fullfunctionName += functionName;
        return myClosures.find(fullfunctionName) != myClosures.end();
    }

protected:

    using ArgsArchive = typename Serialization::OutputArchive;
    using ResultArchive = typename Serialization::InputArchive;

    /**
      * @brief Call a function of a remote service.
      * @param funcName the method name, must match the method in the ServiceInterface class
      * @param c the result closure (callback), which is called when a response is received from the service
      * @param args the method arguments, which must match the signature of the method in the ServiceInterface class
      */
    template<typename ResT, typename... Args>
    void send_call(const char* funcName, const Closure<ResT>& c, Args... args)
    {
        auto result = prepare_call_message(funcName, std::forward<Args>(args)...);
        const std::string& fullFuncName = result.first;
        std::string& msg = result.second;
        if (myClosures.find(fullFuncName) == myClosures.end()) {
            //detail::dumpMessage("send msg:", msg);
            Error err = details::Messenger::write_message_with_header(*myTransport, msg);
            if (err) {
                log<error>(O_LOG_TOKEN, "error - %s", err.message().c_str());
                Result<ResT> res(err);
                c(res);
            } else {
                enqueue_closure(fullFuncName, c);      //new (function copy) for non-one-way methods
            }
        } else if (myCallQueue.can_enqueue(fullFuncName)) {
            myCallQueue.enqueue_call(fullFuncName, [this, msg, c](const std::string& fullMetName,
                                                                  cercall::Transport& t) mutable {
                //detail::dumpMessage("send msg:", msg);
                details::Messenger::write_message_with_header(t, msg);
                enqueue_closure(fullMetName, c);      //new (function copy) for non-one-way methods
            });
        } else {
            throw std::runtime_error("cercall::Client::send_call: the limit of queueing function "
                                        "calls is exceeded");
        }
   }

    /**
      * Overload for one-way calls.
      */
    template<typename... Args>
    void send_call(const char* funcName, Args... args)
    {
        std::string msg = prepare_call_message(funcName, std::forward<Args>(args)...).second;
        details::Messenger::write_message_with_header(*myTransport, msg);
    }

    template<typename S = Serialization>
    void create_archives(typename std::enable_if<S::REUSABLE_ARCHIVE>::type* = nullptr)
    {
        myArchives.outArch = Serialization::create_output_archive();
        myArchives.inArch = Serialization::create_input_archive();
    }

    template<typename S = Serialization>
    void create_archives(typename std::enable_if< !S::REUSABLE_ARCHIVE>::type* = nullptr)
    {
    }

    void on_connected(Transport& tr) override
    {
        create_archives();
        myMessenger.init_transport(tr);
    }

    void on_disconnected(Transport&) override
    {
        check_thread_id("cercall::Client::on_disconnected");
    }

    /**
      * Overrides the Transport::Listener member function to call all outstanding call closures
      * with the error that has occurred in the transport layer.
      */
    void on_connection_error(Transport&, const Error& e) override
    {
        check_thread_id("cercall::Client::on_connection_error");
        log<error>(O_LOG_TOKEN, "error - %s", e.message().c_str());
        dispatch_connection_error(e);
    }

private:
    const std::string myBroadcastFuncName = std::string(details::TypeProperties<InterfaceType>::name) + "::broadcast_event";
    const std::string myFuncPrefix = std::string(details::TypeProperties<InterfaceType>::name) + "::";

    typedef std::list<ServiceListener*> ListenerList;

    ListenerList myEventListeners;
    std::shared_ptr<Transport> myTransport;

    struct Archives
    {
        std::unique_ptr<typename Serialization::OutputArchive> outArch;
        std::unique_ptr<typename Serialization::InputArchive> inArch;
    } myArchives;

    details::Messenger myMessenger;
    details::CallQueue<MaxCallsInProgress - 1> myCallQueue;

    typedef std::function<void(ResultArchive& arRes)> ClosureFunction;
    typedef std::unordered_map<std::string, ClosureFunction> ClosureMap;
    ClosureMap myClosures { 4 };
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

    void dispatch_error_to_closure(const std::string& errorMsg, ClosureFunction& cl)
    {
        Serialization::deserialize_call(myArchives.inArch.get(), errorMsg, [this, cl](const std::string&, ResultArchive& arRes){
            cl(arRes);
        });
    }

    void dispatch_connection_error(const cercall::Error& e)
    {
        Result<void> res(e);
        std::string errCallResMsg = Serialization::template serialize_call_result<void>(myArchives.outArch.get(),
                                                                                        "placeholder", res);
        details::Messenger::strip_header(errCallResMsg);

        for (auto& cl : myClosures) {
            if ( !cl.first.empty()) {
                dispatch_error_to_closure(errCallResMsg, cl.second);
            }
        }
        myClosures.clear();
    }

    uint32_t on_incoming_data(Transport& tr, uint32_t dataLenInBuffer) override
    {
        check_thread_id("cercall::Client::on_incoming_data");
        return myMessenger.read(tr, dataLenInBuffer);
    }

    template<typename... Args>
    std::pair<std::string, std::string> prepare_call_message(const char* funcName, Args... args)
    {
        //Also check the preconditions for send_call.
        o_assert(myTransport != nullptr);
        if (myTransport->is_open()) {
            check_thread_id("cercall::Client::prepare_call_message()");
            std::string fullFuncName = myFuncPrefix;
            fullFuncName += funcName;       //new
            return std::make_pair(fullFuncName, Serialization::serialize_call(myArchives.outArch.get(), fullFuncName, std::forward<Args>(args)...));
        } else {
            throw std::runtime_error("cercall::Client::prepare_call_message: transport to service not opened");
        }
    }

    template<typename ResT>
    void enqueue_closure(const std::string& funcName, const Closure<ResT>& cl)
    {
        myClosures[funcName] = [cl](ResultArchive& arRes){        //new
            Result<ResT> result = Serialization::template deserialize_result<ResT>(arRes);
            cl(result);
        };
    }

    details::Messenger::HandlerType make_message_handler()
    {
        auto messageHandler = [this] (Transport& t, const std::string& msg) {
            (void)t;
            o_assert(myTransport.get() == &t);
            Serialization::deserialize_call(myArchives.inArch.get(), msg, [this](const std::string& funcName, ResultArchive& arRes){
                if (funcName == myBroadcastFuncName) {
                    dispatch_event(arRes);
                } else {
                    dispatch_result(funcName, arRes);
                }
            });
        };
        return messageHandler;
    }

    /** Dispatch an event. */
    template<typename E = EventType>
    typename std::enable_if<!std::is_void<E>::value>::type
    dispatch_event(ResultArchive& arEv) const
    {
        Serialization::template deserialize_event<EventType>(arEv, [this](auto ev) {      //C++14 feature used
            for (auto listener : myEventListeners) {
                listener->on_service_event(std::move(ev));
            }
        });
    }

    /** Do nothing for a void event. */
    template<typename E = EventType>
    typename std::enable_if<std::is_void<E>::value>::type
    dispatch_event(ResultArchive&) const
    {
    }

    void dispatch_result(const std::string& funcName, ResultArchive& res)
    {
        auto closureIt = myClosures.find(funcName);
        if (closureIt != myClosures.end()) {
            //std::cout << "call result closure for " << funcName << std::endl;
            auto closure = closureIt->second;
            myClosures.erase(closureIt);    //allow a new method call in this closure
            if (myCallQueue.is_enqueued(funcName)) {
                //this sends the next queued message and enqueues the closure
                myCallQueue.dequeue_call(funcName, *myTransport);
            }
            closure(res);
        } else {
            throw std::runtime_error("cercall::Client: closure not found for function result");
        }
    }

};

}   //namespace cercall

#endif // CERCALL_CLIENT_H
