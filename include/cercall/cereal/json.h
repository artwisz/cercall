/*!
 * \file
 * \brief     Cercall serialization setup for Cereal JSON archives
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

#ifndef CERCALL_USE_CEREAL_JSON_H
#define CERCALL_USE_CEREAL_JSON_H

#include <cercall/cereal/serialization.h>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>

namespace cercall {
namespace cereal {

struct Json : public Serialization<::cereal::JSONInputArchive, ::cereal::JSONOutputArchive, false>
{
    using InputArchive = ::cereal::JSONInputArchive;
    using OutputArchive = ::cereal::JSONOutputArchive;
};

}
}

#endif //CERCALL_USE_CEREAL_JSON_H
