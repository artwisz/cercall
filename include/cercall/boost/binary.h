/*!
 * \file
 * \brief     Cercall serialization setup for Boost binary archives
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

#ifndef CERCALL_USE_BOOST_BINARY_H
#define CERCALL_USE_BOOST_BINARY_H

#include <cercall/boost/serialization.h>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

namespace cercall {
namespace boost {

struct Binary : public Serialization<::boost::archive::binary_iarchive, ::boost::archive::binary_oarchive, true>
{
    using InputArchive = ::boost::archive::binary_iarchive;
    using OutputArchive = ::boost::archive::binary_oarchive;
};

}   //boost
}   //cereal

#endif //CERCALL_USE_BOOST_BINARY_H
