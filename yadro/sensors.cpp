/**
 * @brief Sensors implementation
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

#include "sensors.hpp"
#include <algorithm>
#include <climits>

/**
 * Operator for binary search (using std::lower_bound) sensor by name
 * It should be faster then std::find
 */
bool sensor_t::operator<(const std::string& s) const
{
    return this->name.compare(s) < 0;
}
/**
 * For using binary search vector must be sorted by sensors name.
 */
bool sensor_t::operator<(const sensor_t& o) const
{
    return *this < o.name;
}
/**
 * @brief Check if sensor is in active state
 *
 * @return true if sensor is not disbled
 */
bool sensor_t::isEnabled(void) const
{
    return mask > E_DISABLED;
}

/**
 * @brief Get current sensor state.
 *
 * @return Current sensor state
 */
sensor_t::state_t sensor_t::getState(void) const
{
    if (mask & (1 << E_CRITICAL_HIGH))
    {
        return E_CRITICAL_HIGH;
    }
    else if (mask & (1 << E_WARNING_HIGH))
    {
        return E_WARNING_HIGH;
    }
    else if (mask & (1 << E_WARNING_LOW))
    {
        return E_WARNING_LOW;
    }
    else if (mask & (1 << E_CRITICAL_LOW))
    {
        return E_CRITICAL_LOW;
    }
    else if (mask & (1 << E_NORMAL))
    {
        return E_NORMAL;
    }

    return E_DISABLED;
}

/**
 * @brief Set new sensor state
 *
 * @param newState - New sensor state
 * @param value - Switch on or off this state
 *
 * @return Previous sensor state
 */
sensor_t::state_t sensor_t::setState(sensor_t::state_t newState, bool value)
{
    auto prev = getState();
    if (newState > E_DISABLED)
    {
        if (value)
        {
            mask |= (1 << newState);
        }
        else
        {
            mask &= ~(1 << newState);
        }
    }
    else
    {
        mask = E_DISABLED;
    }
    return prev;
}

    /**
     * Followed includes should containt a vectors with all possible sensors.
     *
     * TODO: Generate content of followed vectors by yaml files from hwmon
     *       at compile time.
     */

#define SENSOR_ENTRY(name)                                                     \
    {                                                                          \
        name, 0, INT_MIN, INT_MAX, INT_MIN, INT_MAX, sensor_t::E_DISABLED      \
    }

#include "sensors/temperature.hpp"
#include "sensors/voltage.hpp"
#include "sensors/tachometers.hpp"
#include "sensors/current.hpp"
#include "sensors/power.hpp"

/**
 * Initialize sensors. Should be called before use vectors of sensors.
 */
void initialize_sensors(void)
{
    // Ensure that vectors is sorted by sensors name.
    std::sort(temperatureSensors.begin(), temperatureSensors.end());
    std::sort(voltageSensors.begin(), voltageSensors.end());
    std::sort(tachometerSensors.begin(), tachometerSensors.end());
    std::sort(currentSensors.begin(), currentSensors.end());
    std::sort(powerSensors.begin(), powerSensors.end());
}

/**
 * Power stat of host
 */
int hostPowerState = -1;

/**
 * @brief Returns sensor array reference depends on sensor type
 */
sensors_arr_t& getSensorsArr(const std::string& type)
{
    switch (type[0])
    {
        case 't':
            return temperatureSensors;
            break;
        case 'v':
            return voltageSensors;
            break;
        case 'c':
            return currentSensors;
            break;
        case 'p':
            return powerSensors;
            break;
        default:
            return tachometerSensors;
            break;
    }
}
/**
 * @brief Returns sensor scale power depends on sensor type
 */
int getSensorPower(const std::string& type)
{
    switch (type[0])
    {
        case 't':
        case 'v':
        case 'c':
        case 'p':
            return -3;

        default:
            return 0;
    }
}
