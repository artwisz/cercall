#
# cmake helper for test coverage
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

find_program(GCOV_PATH gcov)
find_program(GCOVR_PATH gcovr NAMES gcovr)
find_program(SIMPLE_PYTHON_EXECUTABLE python)

if(NOT GCOV_PATH)
    message(FATAL_ERROR "gcov not found")
endif(NOT GCOV_PATH)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(COVERAGE_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage")
    link_libraries(gcov)
else()
    message(WARNING "coverage not supported with this compiler")
endif()

add_custom_target(coverage
    COMMAND ctest
    COMMAND find ${PROJECT_BINARY_DIR} -name process.cpp.gc?? -delete
    COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/doc/coverage
    COMMAND ${GCOVR_PATH} --html --html-details -r ${CMAKE_SOURCE_DIR} -o ${CMAKE_SOURCE_DIR}/doc/coverage/coverage.html
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMENT "Running gcovr to produce code coverage report."
)
