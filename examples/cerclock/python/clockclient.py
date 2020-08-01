#
# Python code using ClockClient C++ code.
# 
# Copyright (c) 2020, Arthur Wisz
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
 
from pyclock import *
from time import sleep
from datetime import timedelta

finish = False

class MyClock(ClockClient):
    def on_get_time_result(self, result):
        print("Current time: ", result)
        
    def on_set_alarm_result(self, result):
        print("Alarm id ", result, " is set")
        
    def on_tick_event(self, tickTime):
        print("tick event, tick time: ", tickTime)
        
    def on_alarm_event(self, alarmId, tag):
        print("Alarm event, alarmId: ", alarmId, ", tag: ", tag)
        if tag == "stopClient":
            global finish
            finish = True               
        
clock = MyClock("127.0.0.1", "4321")

if (clock.open() == True):
    clock.get__time()
    
    clock.set__tick_interval(timedelta(milliseconds=2000))

    clock.set__alarm("stopClient", timedelta(seconds=10))
    
    while not finish:
        sleep(0.1)
        clock.poll__io()
