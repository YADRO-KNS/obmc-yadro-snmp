/**
 * @brief Sensors definitions
 *
 * This file is part of phosphor-snmp project.
 *
 * Copyright (c) 2018 YADRO
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
 *
 * @author: Alexander Filippov <a.filippov@yadro.com>
 */

#pragma once

#include <string>
#include <vector>

struct sensor_t
{
    enum state_t
    {
        E_DISABLED      = 0,
        E_NORMAL        = 1,
        E_WARNING_LOW   = 2,
        E_WARNING_HIGH  = 3,
        E_CRITICAL_LOW  = 4,
        E_CRITICAL_HIGH = 5,
    };

    bool    operator< (const std::string& s) const;
    bool    operator< (const sensor_t&    o) const;

    bool    isEnabled (void) const;
    state_t enable    (bool  s = true);

    std::string         name;
    int                 currentValue;
    int                 warningLow;
    int                 warningHigh;
    int                 criticalLow;
    int                 criticalHigh;
    state_t             state;
};

using sensors_arr_t = std::vector<sensor_t>;

extern int                      hostPowerState;
extern sensors_arr_t            temperatureSensors;
extern sensors_arr_t            voltageSensors;
extern sensors_arr_t            tachometerSensors;
extern sensors_arr_t            currentSensors;
extern sensors_arr_t            powerSensors;

void initialize_sensors(void);

sensors_arr_t&  getSensorsArr       (const std::string & type);

int             getSensorPower      (const std::string & type);
