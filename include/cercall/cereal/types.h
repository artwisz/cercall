/*!
 * \file
 * \brief     Cercall types serialization functions for Cereal
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

#ifndef CERCALL_CEREAL_TYPES_H
#define CERCALL_CEREAL_TYPES_H

#include <cercall/cercall.h>
#include <cereal/cereal.hpp>
#include "cercall/log.h"

namespace cereal {

using cercall::log;
using cercall::debug;

template<class Archive, typename ResT>
void save(Archive& archive, const cercall::Result<ResT>& res)
{
    archive(cereal::make_nvp("error", res.error()));
    //Serialize result value only when no error.
    if (res.error().code() == 0) {
        archive(cereal::make_nvp("value", res.get_value()));
    }
}

template<class Archive, typename ResT>
void load(Archive& archive, cercall::Result<ResT>& res)
{
    cercall::Error error;
    archive(error);
    res.set_error(error);
    if (error.code() == 0) {
        typename std::decay<ResT>::type value;
        archive(value);
        res.set_value(value);
    }
}

template<class Archive>
void save(Archive& archive, const cercall::Result<void>& res)
{
    archive(cereal::make_nvp("error", res.error()));
}

template<class Archive>
void load(Archive& archive, cercall::Result<void>& res)
{
    cercall::Error error;
    archive(error);
    res.set_error(error);
}

template<class Archive>
void save(Archive & archive, cercall::Error const & e)
{
    archive(cereal::make_nvp("code", e.code()));
    archive(cereal::make_nvp("message", e.message()));
}

template<class Archive>
void load(Archive & archive, cercall::Error& e)
{
    int errCode;
    std::string errMsg;
    archive(errCode, errMsg);
    e = cercall::Error(errCode, errMsg);
}

}   //cereal

#endif //CERCALL_CEREAL_TYPES_H
