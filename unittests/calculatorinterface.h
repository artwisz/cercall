/*!
 * \file
 * \brief     Calculator service interface for Cercall tests
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

#ifndef CERCALL_TEST_CALCULATOR_INTERFACE_H
#define CERCALL_TEST_CALCULATOR_INTERFACE_H

#include <cstdint>
#include "cercall/cercall.h"
#if defined(TEST_CEREAL_BINARY)
#include "cercall/cereal/binary.h"
#elif defined(TEST_CEREAL_JSON)
#include "cercall/cereal/json.h"
#elif defined(TEST_BOOST_TEXT)
#include "cercall/boost/text.h"
#elif defined(TEST_BOOST_BINARY)
#include "cercall/boost/binary.h"
#endif

#if defined(TEST_CEREAL_BINARY) || defined(TEST_CEREAL_JSON)
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#elif defined(TEST_BOOST_TEXT) || defined(TEST_BOOST_BINARY)
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/shared_ptr.hpp>
#endif

O_REGISTER_TYPE(CalculatorInterface);

class CalculatorInterface
{
public:
#if defined(TEST_CEREAL_BINARY)
    using Serialization = cercall::cereal::Binary;
#elif defined(TEST_CEREAL_JSON)
    using Serialization = cercall::cereal::Json;
#elif defined(TEST_BOOST_TEXT)
    using Serialization = cercall::boost::Text;
#elif defined(TEST_BOOST_BINARY)
    using Serialization = cercall::boost::Binary;
#endif

    template<typename T>
    using Closure = typename cercall::Closure<T>;

    virtual void add(int8_t a, int16_t b, int32_t c, Closure<int32_t> cl) = 0;

    virtual void add_and_delay_result(int32_t a, int32_t b, Closure<int32_t> cl) = 0;

    virtual void add_vector(const std::vector<int32_t>& a, const std::vector<int32_t>& b, Closure<std::vector<int64_t>> cl) = 0;

    virtual void close_service(void) = 0;   ///< One-way function.

    virtual void get_connected_clients_count(Closure<size_t> cl) = 0;
};

#endif //CERCALL_TEST_CALCULATOR_INTERFACE_H
