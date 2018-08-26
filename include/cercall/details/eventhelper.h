/*!
 * \file
 * \brief     Cercall details - event helper traits
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

#ifndef CERCALL_EVENT_HELPER_H
#define CERCALL_EVENT_HELPER_H

namespace cercall {

namespace details {
template<typename SI, typename Enable = void>
struct complement_event_type
{
    using type = void;
};

template<typename SI>
struct complement_event_type<SI, typename std::enable_if<!std::is_void<typename SI::EventType>::value>::type>
{
    using type = typename SI::EventType;
};

}   //namespace details
}   //namespace cercall

#endif // CERCALL_EVENT_HELPER_H
