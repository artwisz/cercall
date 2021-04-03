#
# Main cmake configuration for Cercall
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

project(cercall)

cmake_minimum_required(VERSION 3.1.0)

enable_testing()

install(DIRECTORY include/cercall DESTINATION include)

option(USE_BOOST_ASIO "use Boost.Asio" false)
option(USE_CEREAL "use Cereal serialization" true)
option(BUILD_TESTS "build unit tests" false)
option(USE_PYBIND11 "build pybind example" false)

if (${USE_BOOST_ASIO})
    set(USE_ASIO_STANDALONE false)
    find_path(BOOST_ROOT NAMES boost/asio.hpp PATHS ${CMAKE_SOURCE_DIR}/../boost)
    find_package(Boost 1.67 REQUIRED COMPONENTS system serialization)
    message(STATUS "boost headers found in ${Boost_INCLUDE_DIRS}")
    include_directories(${Boost_INCLUDE_DIRS})
    set(BOOST_SYSTEM -lboost_system)
    set(BOOST_SERIALIZATION -lboost_serialization)
    link_directories(${Boost_LIBRARY_DIRS})
else (${USE_BOOST_ASIO})
    set(USE_ASIO_STANDALONE true)
    add_definitions(-DASIO_STANDALONE)
    find_path(ASIO_INCLUDE_DIR NAMES asio.hpp PATHS ${CMAKE_SOURCE_DIR}/../asio PATH_SUFFIXES include)
    message(STATUS "ASIO_INCLUDE_DIR is ${ASIO_INCLUDE_DIR}")
    if(EXISTS "${ASIO_INCLUDE_DIR}")
        message(STATUS "asio library found in ${ASIO_INCLUDE_DIR}")
        include_directories(${ASIO_INCLUDE_DIR})
    else(EXISTS "${ASIO_INCLUDE_DIR}")
        message(FATAL_ERROR "asio library (asio.hpp) could not be found.")
    endif(EXISTS "${ASIO_INCLUDE_DIR}")
endif (${USE_BOOST_ASIO})

if (${USE_CEREAL})
    find_path(CEREAL_INCLUDE_DIR NAMES cereal/cereal.hpp PATHS ${CMAKE_SOURCE_DIR}/../cereal PATH_SUFFIXES include)

    if(EXISTS "${CEREAL_INCLUDE_DIR}")
        message(STATUS "cereal library found in ${CEREAL_INCLUDE_DIR}")
        include_directories(AFTER ${CEREAL_INCLUDE_DIR})
        add_definitions(-DHAS_CEREAL)
    else(EXISTS "${CEREAL_INCLUDE_DIR}")
        message(FATAL_ERROR "cereal library (cereal.hpp) could not be found.")
    endif(EXISTS "${CEREAL_INCLUDE_DIR}")
else (${USE_CEREAL})
    if (NOT ${USE_BOOST_ASIO})
        message(FATAL_ERROR "No serialization library to use")
    endif (NOT ${USE_BOOST_ASIO})
endif (${USE_CEREAL})

if (${USE_PYBIND11})
add_subdirectory(pybind11)
endif (${USE_PYBIND11})

file(GLOB_RECURSE CERCALL_HEADERS "include/*.h")

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-Wall -g -Wextra -Wshadow -ftemplate-backtrace-limit=64 -Wno-implicit-fallthrough ${CMAKE_CXX_FLAGS}")

if (WIN32)
    macro(get_WIN32_WINNT version)
        if (CMAKE_SYSTEM_VERSION)
            set(ver ${CMAKE_SYSTEM_VERSION})
            string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
            string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
            # Check for Windows 10, b/c we'll need to convert to hex 'A'.
            if ("${verMajor}" MATCHES "10")
                set(verMajor "A")
                string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
            endif ("${verMajor}" MATCHES "10")
            # Remove all remaining '.' characters.
            string(REPLACE "." "" ver ${ver})
            # Prepend each digit with a zero.
            string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
            set(${version} "0x${ver}")
        endif(CMAKE_SYSTEM_VERSION)
    endmacro(get_WIN32_WINNT)

    get_WIN32_WINNT(ver)
    add_definitions(-D_WIN32_WINNT=${ver})
endif(WIN32)

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads)

add_subdirectory(examples)
if (${BUILD_TESTS})
add_subdirectory(unittests)
else (${BUILD_TESTS})
add_subdirectory(unittests EXCLUDE_FROM_ALL)
endif (${BUILD_TESTS})
