/*!
 * \file
 * \brief     Cercall library details - function dictionary class
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


#ifndef CERCALL_FUNCTIONDICT_H
#define CERCALL_FUNCTIONDICT_H

#include <map>
#include "cercall/cercall.h"
#include "cercall/details/typeutil.h"

namespace cercall {
namespace details {

/**
 * An instance of this template is created for every Cercall function that is added to a function
 * dictionary of a Cercall service.
 * The class deserializes the function arguments and calls the service function.
 */
template<typename F, class SrvIfc, typename Serialization, bool OneWay, typename R, typename ArgsTuple>
class FunctionCaller
{
    template<bool OW, typename Enable = void>
    struct ClosureChoice
    {
        template<typename T>
        using Closure = typename cercall::Closure<T>;
    };

    /* Use empty closure for one-way functions. */
    template<bool OW>
    struct ClosureChoice<OW, typename std::enable_if<OW>::type>
    {
        template<typename T>
        struct Closure
        {
            template<typename FF>
            Closure(FF) {}

            template<typename FF>
            Closure(FF, const std::shared_ptr<cercall::Transport>&) {}
        };
    };

    //The compiler will choose this type when used in the FunctionCaller class.
    //This is to avoid SFINAE on deserialize_args functions.
    template <typename T>
    using Closure = typename ClosureChoice<OneWay>::template Closure<T>;

    using ArgArch = typename Serialization::InputArchive;
    using ResArch = typename Serialization::OutputArchive;

public:
    using ResultHandler = typename std::function<void(std::string& resultMsg)>;

    FunctionCaller(F f) : myFunc { f } {}

    void operator()(SrvIfc& obj, std::shared_ptr<Transport>& clTr, ArgArch& args, ResArch* resAr,
                    const std::string& funcName, ResultHandler& rh)
    {
        //The Closure object which is the last parameter of a service function.
        Closure<R> closure {[resAr, funcName, rh] (const Result<R>& r) {
            cercall::Result<R> res(r);
            std::string resMsg = Serialization::template serialize_call_result<R>(resAr, funcName, res);
            rh(resMsg);
        }, clTr};
        deserialize_args(obj, args, closure, ArgsTuple{});
    }

private:

    F myFunc;

    template<class Head, class... Tail, class... Collected>
    void deserialize_args(SrvIfc& obj, ArgArch& args, Closure<R>& cl, type_tuple<Head, Tail...>, Collected... c)
    {
        deserialize_args<Tail...>(obj, args, cl, type_tuple<Tail...>{}, c..., get<Head>(args));
    }

    template<class... Collected>
    void deserialize_args(SrvIfc& obj, ArgArch&, Closure<R>& cl, type_tuple<>, Collected... c)
    {
        call_func(obj, cl, std::integral_constant<bool, OneWay>(), c...);
    }

    template<class... Collected>
    void call_func(SrvIfc& obj, Closure<R>& cl, std::false_type, Collected... c)
    {
        myFunc(obj, c..., cl);
    }

    //Overload for the one-way service functions.
    template<class... Collected>
    void call_func(SrvIfc& obj, Closure<R>&, std::true_type, Collected... c)
    {
        //One-way functions don't have the Closure parameter.
        myFunc(obj, c...);
    }

    //T must have a default constructor.
    template<class T>
    static typename std::remove_reference<T>::type get(ArgArch& args)
    {
        typename std::decay<T>::type t;
        Serialization::template deserialize_arg(args, t);
        return t;
    }
};

template<class SrvIfc, typename Serialization, bool OneWay, typename ...Ts, typename F>
FunctionCaller<F, SrvIfc, Serialization, OneWay, Ts...> make_function_caller(F f)  {
    return {f};
}

template<typename SI>
struct CompareMemFn
{
    bool operator () (void (SI::*f1)(), void (SI::*f2)())
    {
        return &f1 < &f2;
    }
};

template<typename SI, typename Serialization>
class FunctionDict
{
    using ArgsArchive = typename Serialization::InputArchive;
    using ResultArchive = typename Serialization::OutputArchive;
    using ResultHandler = typename std::function<void(std::string&)>;
    typedef std::function<void(SI& obj, std::shared_ptr<Transport>& clTr, ArgsArchive& args, ResultArchive* resAr,
                               const std::string& functionName, ResultHandler& h)> DictFunction;

    struct DictEntry
    {
        DictFunction func;
        bool oneWay = false;
    };

    typedef std::map<std::string, DictEntry> FuncDictType;
    typedef void (SI::*GenericMemberFunctionType)();
    typedef std::map<GenericMemberFunctionType, typename FuncDictType::const_iterator, CompareMemFn<SI>> MemberFunctionMap;

    template <class, bool, class = void>
    struct result_type_traits
    {
        using type = void;
        template<typename ...Args>
        using ArgsTuple = type_tuple<Args...>;
    };

    template <class T, bool OneWay>
    struct result_type_traits<T, OneWay, typename std::enable_if<!OneWay>::type>
    {
        using type = typename T::ResultType;
        template<typename ...Args>
        using ArgsTuple = typename remove_last_type_of<Args...>::type;
    };

public:

    template<bool OneWay, typename ...Args>
    void add_function(const std::string& functionName, void (SI::*function)(Args...))
    {
        using LastArgType = typename last_type_of<Args...>::type;
        using ResT = typename result_type_traits<LastArgType, OneWay>::type;
        using ArgsTuple = typename result_type_traits<LastArgType, OneWay>::template ArgsTuple<Args...>;

        myFunctionDict[functionName].func = make_function_caller<SI, Serialization, OneWay, ResT, ArgsTuple>(std::mem_fn(function));
        myFunctionDict[functionName].oneWay = OneWay;
        myFunctionMap[reinterpret_cast<GenericMemberFunctionType>(function)] = myFunctionDict.find(functionName);
    }

    template<bool OneWay>
    void add_function(const std::string &functionName, void (SI::*function)())
    {
        static_assert(OneWay == true, "parameterless service function must be a one-way function");

        using ArgsTuple = details::type_tuple<>;

        myFunctionDict[functionName].func = make_function_caller<SI, Serialization, OneWay, void, ArgsTuple>(std::mem_fn(function));
        myFunctionDict[functionName].oneWay = OneWay;
        myFunctionMap[reinterpret_cast<GenericMemberFunctionType>(function)] = myFunctionDict.find(functionName);
    }

    template<bool OneWay, typename BaseInterface, typename ...Args>
    void add_function(const std::string& functionName, void (BaseInterface::*function)(Args...))
    {
        using LastArgType = typename last_type_of<Args...>::type;
        using ResT = typename result_type_traits<LastArgType, OneWay>::type;
        using ArgsTuple = typename result_type_traits<LastArgType, OneWay>::template ArgsTuple<Args...>;

        myFunctionDict[functionName].func
                = make_function_caller<SI, Serialization, OneWay, ResT, ArgsTuple>(std::mem_fn(function));
        myFunctionDict[functionName].oneWay = OneWay;
        myFunctionMap[reinterpret_cast<GenericMemberFunctionType>(function)] = myFunctionDict.find(functionName);
    }

    void call_function(std::shared_ptr<Transport>& clTr, const std::string& functionName, SI& obj,
                       ResultArchive* resAr, ArgsArchive& args, ResultHandler rh)
    {
        find(functionName)->second.func(obj, clTr, args, resAr, functionName, rh);
    }

    template<typename ...Args>
    const std::string& find_function_name(void (SI::*function)(Args...))
    {
        auto found = myFunctionMap.find(reinterpret_cast<GenericMemberFunctionType>(function));
        if (found == myFunctionMap.end()) {
            throw std::logic_error("function name could not be found");
        } else {
            return found->second->first;
        }
    }

    bool is_one_way(const std::string& functionName)
    {
        return find(functionName)->second.oneWay;
    }

private:
    FuncDictType myFunctionDict;
    MemberFunctionMap myFunctionMap;

    typename FuncDictType::iterator find(const std::string& functionName)
    {
        auto found = myFunctionDict.find(functionName);
        if (found == myFunctionDict.end()) {
            std::string err = functionName + " not found in function dictionary";
            throw std::logic_error(err);
        }
        return found;
    }
};

}   //namespace details
}   //namespace cercall

#endif // CERCALL_FUNCTIONDICT_H

