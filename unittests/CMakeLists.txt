#
# cmake configuration for Cercall unit tests
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

project(cercall_tests)

include(${CMAKE_SOURCE_DIR}/cmake/coverage.cmake)

#if(CMAKE_BUILD_TYPE MATCHES DEBUG)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
#endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

set(gtest_path ${CMAKE_SOURCE_DIR}/../googletest/googletest)

if(NOT EXISTS ${gtest_path})
    unset(gtest_path)
    if (NOT WIN32)
        set(gtest_path "/usr/src/gtest")
        if(NOT EXISTS ${gtest_path})
            unset(gtest_path)
        endif(NOT EXISTS ${gtest_path})
    endif(NOT WIN32)
endif(NOT EXISTS ${gtest_path})

if(gtest_path)
    include_directories(${CMAKE_SOURCE_DIR} ${gtest_path}/include ${gtest_path})
else(gtest_path)
    message(FATAL_ERROR "missing googletest library")
endif(gtest_path)

include_directories(${CMAKE_SOURCE_DIR}/utils)

include(${gtest_path}/cmake/internal_utils.cmake)

config_compiler_and_linker()  # Defined in internal_utils.cmake.

set(GTEST_CXX_FLAGS "${cxx_strict}")

cxx_library(gtest "${GTEST_CXX_FLAGS}" ${gtest_path}/src/gtest-all.cc)
cxx_library(gtest_main "${GTEST_CXX_FLAGS}" testmain.cpp)
target_link_libraries(gtest_main gtest ${BOOST_SYSTEM} ${CMAKE_THREAD_LIBS_INIT})

if (${USE_BOOST_ASIO})
set(TEST_LIBS "${CMAKE_THREAD_LIBS_INIT} ${BOOST_SYSTEM}")
else (${USE_BOOST_ASIO})
set(TEST_LIBS "${CMAKE_THREAD_LIBS_INIT}")
endif (${USE_BOOST_ASIO})

if (${USE_CEREAL})
set(CXX_TEST_BINARY_FLAGS "${cxx_default} -DTEST_CEREAL_BINARY")

cxx_executable_with_flags(calltest_server "${CXX_TEST_BINARY_FLAGS}" "${TEST_LIBS}"
                            calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(calltest "${CXX_TEST_BINARY_FLAGS}" gtest_main calltest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

cxx_executable_with_flags(eventstest_server "${CXX_TEST_BINARY_FLAGS}" "${TEST_LIBS}"
                            eventstestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(eventstest "${CXX_TEST_BINARY_FLAGS}" gtest_main eventstest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

cxx_executable_with_flags(errorstest_server "${CXX_TEST_BINARY_FLAGS}" "${TEST_LIBS}"
                            calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(errorstest "${CXX_TEST_BINARY_FLAGS}" gtest_main errorstest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

set(CXX_TEST_JSON_FLAGS "${cxx_default} -DTEST_CEREAL_JSON")

cxx_executable_with_flags(calltestjson_server "${CXX_TEST_JSON_FLAGS}" "${TEST_LIBS}"
                            calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(calltestjson "${CXX_TEST_JSON_FLAGS}" gtest_main calltest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

cxx_executable_with_flags(eventstestjson_server "${CXX_TEST_JSON_FLAGS}" "${TEST_LIBS}"
                            eventstestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(eventstestjson "${CXX_TEST_JSON_FLAGS}" gtest_main eventstest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

cxx_executable_with_flags(errorstestjson_server "${CXX_TEST_JSON_FLAGS}" "${TEST_LIBS}"
                            calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(errorstestjson "${CXX_TEST_JSON_FLAGS}" gtest_main errorstest.cpp process.cpp
                            ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

add_test(NAME calltest-cereal-binary COMMAND calltest)
add_test(NAME eventstest-cereal-binary COMMAND eventstest)
add_test(NAME errorstest-cereal-binary COMMAND errorstest)

add_test(NAME calltest-cereal-json COMMAND calltestjson)
add_test(NAME eventstest-cereal-json COMMAND eventstestjson)
add_test(NAME errorstest-cereal-json COMMAND errorstestjson)

endif (${USE_CEREAL})

if (${USE_BOOST_ASIO})
set(CXX_TEST_BOOST_TEXT_FLAGS "${cxx_default} -DTEST_BOOST_TEXT")

set(TEST_LIBS "${CMAKE_THREAD_LIBS_INIT} ${BOOST_SYSTEM} ${BOOST_SERIALIZATION}")

### Text serialization

cxx_executable_with_flags(calltestboosttext_server "${CXX_TEST_BOOST_TEXT_FLAGS}" "${TEST_LIBS}"
                          calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(calltestboosttext "${CXX_TEST_BOOST_TEXT_FLAGS}" gtest_main
                          calltest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

target_link_libraries(calltestboosttext ${BOOST_SERIALIZATION})

cxx_executable_with_flags(eventstestboosttext_server "${CXX_TEST_BOOST_TEXT_FLAGS}" "${TEST_LIBS}"
                          eventstestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(eventstestboosttext "${CXX_TEST_BOOST_TEXT_FLAGS}" gtest_main
                          eventstest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
target_link_libraries(eventstestboosttext ${BOOST_SERIALIZATION})

cxx_executable_with_flags(errorstestboosttext_server "${CXX_TEST_BOOST_TEXT_FLAGS}" "${TEST_LIBS}"
                          calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(errorstestboosttext "${CXX_TEST_BOOST_TEXT_FLAGS}" gtest_main
                          errorstest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
target_link_libraries(errorstestboosttext ${BOOST_SERIALIZATION})


### Binary serialization

set(CXX_TEST_BOOST_BINARY_FLAGS "${cxx_default} -DTEST_BOOST_BINARY")

cxx_executable_with_flags(calltestboostbinary_server "${CXX_TEST_BOOST_BINARY_FLAGS}" "${TEST_LIBS}"
                          calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(calltestboostbinary "${CXX_TEST_BOOST_BINARY_FLAGS}" gtest_main
                          calltest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})

target_link_libraries(calltestboostbinary ${BOOST_SERIALIZATION})

cxx_executable_with_flags(eventstestboostbinary_server "${CXX_TEST_BOOST_BINARY_FLAGS}" "${TEST_LIBS}"
                          eventstestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(eventstestboostbinary "${CXX_TEST_BOOST_BINARY_FLAGS}" gtest_main
                          eventstest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
target_link_libraries(eventstestboostbinary ${BOOST_SERIALIZATION})

cxx_executable_with_flags(errorstestboostbinary_server "${CXX_TEST_BOOST_BINARY_FLAGS}" "${TEST_LIBS}"
                          calltestserver.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
cxx_executable_with_flags(errorstestboostbinary "${CXX_TEST_BOOST_BINARY_FLAGS}" gtest_main
                          errorstest.cpp process.cpp ${CMAKE_SOURCE_DIR}/utils/program.cpp ${CERCALL_HEADERS})
target_link_libraries(errorstestboostbinary ${BOOST_SERIALIZATION})


add_test(NAME calltest-boost-text COMMAND calltestboosttext)
add_test(NAME eventstest-boost-text COMMAND eventstestboosttext)
add_test(NAME errorstest-boost-text COMMAND errorstestboosttext)

add_test(NAME calltest-boost-binary COMMAND calltestboostbinary)
add_test(NAME eventstest-boost-binary COMMAND eventstestboostbinary)
add_test(NAME errorstest-boost-binary COMMAND errorstestboostbinary)

endif (${USE_BOOST_ASIO})
