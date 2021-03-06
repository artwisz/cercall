/*!
 * \file
 * \brief     Cercall library - serialization adapter for Boost
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


#ifndef CERCALL_USE_BOOST_SERIALIZATION_H
#define CERCALL_USE_BOOST_SERIALIZATION_H

#include <string>
#include "cercall/boost/types.h"
#include "cercall/details/typeprops.h"
#include "cercall/details/messenger.h"
#include <sstream>
#include <boost/archive/basic_archive.hpp>
#include <boost/serialization/unique_ptr.hpp>

namespace cercall {
namespace boost {

static const std::string emptyString {};
static thread_local std::ostringstream outStringStream;
static thread_local std::istringstream inStringStream;

template<class InputArchive, class OutputArchive, bool Reusable>
struct Serialization
{
    static constexpr bool REUSABLE_ARCHIVE = Reusable;

    static std::unique_ptr<InputArchive> create_input_archive()
    {
        if (Reusable) {
            return std::make_unique<InputArchive>(inStringStream, get_arch_option());
        } else {
            return nullptr;
        }
    }

    static std::unique_ptr<OutputArchive> create_output_archive()
    {
        if (Reusable) {
            return std::make_unique<OutputArchive>(outStringStream, get_arch_option());
        } else {
            return nullptr;
        }
    }

    template<typename FirstArg, typename... Args>
    static void serialize_args(OutputArchive& ar, const FirstArg& arg, const Args... args)
    {
        ar & arg;
        serialize_args(ar, args...);
    }

    template<typename LastArg>
    static void serialize_args(OutputArchive& ar, const LastArg& arg)
    {
        ar & arg;
    }

    static void serialize_args(OutputArchive&)
    {
    }

    template<typename ...Args>
    static std::string serialize_call(OutputArchive* ar, const std::string& functionName, Args... args)
    {
        outStringStream.str(emptyString);
        outStringStream.clear();
        details::Messenger::reserve_message_header(outStringStream);
        if (ar == nullptr) {
            OutputArchive arMsg(outStringStream, get_arch_option());     //heavy
            arMsg & ::boost::serialization::make_nvp("func", functionName);
            serialize_args(arMsg, args...);
        } else {
            (*ar) & ::boost::serialization::make_nvp("func", functionName);
            serialize_args((*ar), args...);
        }
        return outStringStream.str();
    }

    template<typename ResultT>
    static std::string serialize_call_result(OutputArchive* ar, const std::string& functionName, cercall::Result<ResultT>& res)
    {
        outStringStream.str(emptyString);
        outStringStream.clear();
        details::Messenger::reserve_message_header(outStringStream);
        if (ar == nullptr) {
            OutputArchive resultArch(outStringStream, get_arch_option());
            resultArch & ::boost::serialization::make_nvp("func", functionName);
            resultArch & ::boost::serialization::make_nvp("result", res);
        } else {
            (*ar) & ::boost::serialization::make_nvp("func", functionName);
            (*ar) & ::boost::serialization::make_nvp("result", res);
        }
        return outStringStream.str();
    }

    template<typename EventT>
    static std::string serialize_event(OutputArchive* ar, const std::string& funcName, const EventT& ev)
    {
        outStringStream.str(emptyString);
        outStringStream.clear();
        details::Messenger::reserve_message_header(outStringStream);
        if (ar == nullptr) {
            OutputArchive resultArch(outStringStream, get_arch_option());
            resultArch & ::boost::serialization::make_nvp("func", funcName);
            resultArch & ::boost::serialization::make_nvp("result", ev);
        } else {
            (*ar) & ::boost::serialization::make_nvp("func", funcName);
            (*ar) & ::boost::serialization::make_nvp("result", ev);
        }
        return outStringStream.str();
    }

    template<typename ResultHandler>
    static void deserialize_call(InputArchive* ar, const std::string& msg, ResultHandler handler)
    {
        inStringStream.str(msg);
        inStringStream.clear();     //clear ios flags
        std::string funcName;
        if (ar == nullptr) {
            InputArchive arRes (inStringStream, get_arch_option());     //heavy
            arRes & ::boost::serialization::make_nvp("func", funcName);
            handler(funcName, arRes);
        } else {
            (*ar) & ::boost::serialization::make_nvp("func", funcName);
            handler(funcName, (*ar));
        }
    }

    template<typename T>
    static void deserialize_arg(InputArchive& arArgs, T& t)
    {
        arArgs & t;
    }

    template<typename ResultT>
    static typename std::enable_if<!std::is_void<ResultT>::value, cercall::Result<ResultT>>::type
    deserialize_result(InputArchive& arRes)
    {
        cercall::Result<ResultT> result;
        arRes & ::boost::serialization::make_nvp("result", result);
        return result;
    }

    template<typename ResultT>
    static typename std::enable_if<std::is_void<ResultT>::value, cercall::Result<void>>::type
    deserialize_result(InputArchive& arRes)
    {
        cercall::Result<ResultT> result;
        arRes & ::boost::serialization::make_nvp("result", result);
        return result;
    }

    /** Deserialize a polymorphic event. */
    template<typename E, typename EventHandler>
    static typename std::enable_if<std::is_polymorphic<E>::value>::type
    deserialize_event(InputArchive& arEv, EventHandler handler)
    {
        std::unique_ptr<E> ev;
        arEv & ev;
        handler(std::move(ev));
    }

    /** Deserialize a non-polymorphic event. */
    template<typename E, typename EventHandler>
    static typename std::enable_if<!std::is_polymorphic<E>{}>::type
    deserialize_event(InputArchive& arEv, EventHandler handler)
    {
        E ev;
        arEv & ev;
        handler(ev);
    }

private:

    static unsigned int get_arch_option()
    {
        return Reusable ? (::boost::archive::no_header | ::boost::archive::no_codecvt) : 0u;
    }

};

}   //namespace boost
}   //namespace cercall

#endif // CERCALL_USE_BOOST_SERIALIZATION_H
