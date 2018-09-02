/*!
 * \file
 * \brief     Main Cercall library header
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

#ifndef CERCALL_MAIN_H
#define CERCALL_MAIN_H

#include <functional>
#include <type_traits>
#include "cercall/error.h"
#include "cercall/details/typeprops.h"
#include "cercall/details/cpputil.h"
#include "cercall/log.h"

/** Using cercall in multiple threads is not supported yet. */
#define O_ENSURE_SINGLE_THREAD

namespace cercall {

/** @defgroup Cercall
 * A library for developing microservices with serialized function calls.
 */

namespace details {

template<typename T, typename Enable = void>
class BaseResult;

template<typename T>
class BaseResult<T, typename std::enable_if<!std::is_void<T>::value>::type>
{
public:
    const T& get_value() const { return value; }
    void set_value(const T& v) { value = v; }

protected:
    BaseResult(const T& v) : value(v) {}
    BaseResult() {}

    T value;
};

template<typename T>
class BaseResult<T, typename std::enable_if<std::is_void<T>::value>::type>
{
};

}  //details

/**
 * @brief Result of a Cercall function call.
 *
 * The result consists of a returned value (may be void) and an error code.
 */
template<typename T>
class Result : public details::BaseResult<T>
{
public:
    template<typename U = T, typename SFINAE = typename std::enable_if<!std::is_void<U>::value>::type>
    explicit Result(const U& r, const Error& e = Error())
        : details::BaseResult<U>(r), myError(e) {}

    template<typename U = T, typename = typename std::enable_if<std::is_void<U>::value>::type>
    explicit Result() : myError() {}

    explicit Result(const Error& e) : myError(e) {}

    explicit Result() {}

    const Error& error() const { return myError; }
    void set_error(const Error& e) { myError = e; }

    explicit operator bool() const noexcept
    {
        return !error().operator bool();
    }

private:
    Error myError;
};

class Transport;

template<typename ResT>
class Closure : public std::function<void(Result<ResT>)>
{
public:
    using ResultType = ResT;

    Closure() : std::function<void(Result<ResT>)>(nullptr) {}

    template<typename F>
    Closure(F f) : std::function<void(Result<ResT>)>(f) {}

    template<typename F>
    Closure(F f, const std::shared_ptr<Transport>& clTr) : std::function<void(Result<ResT>)>(f), myTransport(clTr) {}

    /* Non-void result variant. */
    template<typename R = ResT, typename std::enable_if<!std::is_void<R>::value>::type* = nullptr>
    void operator() (const Result<R>& res) const
    {
        std::function<void(Result<ResT>)>::operator()(res);
    }

    /* Void result variant. */
    template<typename R = ResT, typename std::enable_if<std::is_void<R>::value>::type* = nullptr>
    void operator() (const Result<R>& res = Result<R>()) const
    {
        std::function<void(Result<void>)>::operator()(res);
    }

    /* Non-void result variant. */
    template<typename R = ResT, typename std::enable_if<!std::is_void<R>::value>::type* = nullptr>
    void operator() (const R& res) const
    {
        std::function<void(Result<ResT>)>::operator() (Result<ResT>(res, Error()));
    }

    /**
     * \brief Get the transport of the calling client.
     * \return the transport of the calling client on the service side, or null pointer on the client side.
     * The service can use the returned value to distinguish its clients and maintain any meaningful per-client state.
     * The function returns null pointer on the client side.
     */
    const std::shared_ptr<Transport>& get_client_transport() const
    {
        return myTransport;
    }

private:
    std::shared_ptr<Transport> myTransport;
};

}   //namespace cercall

#endif // CERCALL_MAIN_H
