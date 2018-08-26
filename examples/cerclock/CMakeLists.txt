#
# cmake configuration for Cercall clock example
#
#  Copyright (c) 2018, Arthur Wisz
#  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

add_definitions(-DCERCALL_THREAD_PROTECTION)

add_executable(clockservice clockservice.cpp clockinterface.h ${CERCALL_HEADERS})
add_executable(clockclient clockclient.cpp)

if (CMAKE_USE_PTHREADS_INIT)
    target_link_libraries(clockservice ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(clockclient ${CMAKE_THREAD_LIBS_INIT})
endif(CMAKE_USE_PTHREADS_INIT)

if (WIN32)
    set_target_properties(clockservice PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
    set_target_properties(clockclient PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif(WIN32)

if (NOT ${USE_ASIO_STANDALONE})
    target_link_libraries(clockservice ${BOOST_SYSTEM} ${BOOST_SERIALIZATION})
    target_link_libraries(clockclient ${BOOST_SYSTEM} ${BOOST_SERIALIZATION})
endif (NOT ${USE_ASIO_STANDALONE})
