/*!
 * \file
 * \brief     Cercall tests main
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


#include "gtest/gtest.h"
#include "program.h"
#include <vector>

std::vector<const char*> programArguments;

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    for (int i = 0; i < argc; ++i) {
        programArguments.push_back(argv[i]);
    }

    printf("Run all tests()\n");
    return RUN_ALL_TESTS();
}
