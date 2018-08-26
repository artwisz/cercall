/*!
 * \file
 * \brief     Cercall Call Queue class
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


#ifndef CERCALL_CALLQUEUE_H
#define CERCALL_CALLQUEUE_H

#include <string>
#include <unordered_map>
#include <list>
#include <memory>

namespace cercall {
namespace details {

template<unsigned CallQueueLimit>
class CallQueue
{
public:
    using DequeueAction = std::function<void(const std::string&, cercall::Transport&)>;
    bool can_enqueue(const std::string& funcName)
    {
        if (CallQueueLimit == 0) {
            return false;
        } else if (myQueueMap.find(funcName) == myQueueMap.end()) {
            return true;
        } else {
            return myQueueMap[funcName]->size() < CallQueueLimit;
        }
    }

    void enqueue_call(const std::string& funcName, DequeueAction dequeueAction)
    {
        if (myQueueMap.find(funcName) == myQueueMap.end()) {
            myQueueMap[funcName] = std::shared_ptr<ActionQueue>(new ActionQueue());
        }
        myQueueMap[funcName]->push_back(dequeueAction);
    }

    bool is_enqueued(const std::string& funcName)
    {
        if (CallQueueLimit == 0) {
            return false;
        } else if (myQueueMap.empty() || myQueueMap.find(funcName) == myQueueMap.end()) {
            return false;
        } else {
            return myQueueMap[funcName]->size() > 0;
        }
    }

    void dequeue_call(const std::string& funcName, cercall::Transport& t)
    {
        o_assert(myQueueMap.find(funcName) != myQueueMap.end());
        myQueueMap[funcName]->front()(funcName, t);
        myQueueMap[funcName]->pop_front();
    }

private:
    using ActionQueue = std::list<DequeueAction>;
    std::unordered_map<std::string, std::shared_ptr<ActionQueue>> myQueueMap;
};

}   //namespace details
}   //namespace cercall

#endif // CERCALL_CALLQUEUE_H
