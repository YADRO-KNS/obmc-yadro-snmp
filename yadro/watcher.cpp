/**
 * @brief DBus objects watcher implementation
 *
 * This file is part of phosphor-snmp project.
 *
 * Copyright (c) 2018 YADRO (KNS Group LLC)
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

#include <math.h>                           // powf()
#include <tracing.hpp>                      // TRACE, TRACE_ERROR

#include "sdbus-remote.hpp"                 // sdbus::bus::open_system()
#include "watcher.hpp"                      // dbuswatcher

#define OBJECT_MAPPER_IFACE  "xyz.openbmc_project.ObjectMapper"
#define OBJECT_MAPPER_PATH   "/xyz/openbmc_project/object_mapper"
#define DBUS_PROPETIES_IFACE "org.freedesktop.DBus.Properties"
#define SENSORS_FOLDER       "/xyz/openbmc_project/sensors"
#define POWER_STATE_IFACE    "org.openbmc.control.Power"
#define POWER_STATE_PATH     "/org/openbmc/control/power0"

#define SENSOR_VALUE    "xyz.openbmc_project.Sensor.Value"
#define SENSOR_WARNING  "xyz.openbmc_project.Sensor.Threshold.Warning"
#define SENSOR_CRITICAL "xyz.openbmc_project.Sensor.Threshold.Critical"

#define SCALE_AND_ROUND(v, s) static_cast<int>((v + 0.5)/s)

/**
* @brief dbuswatcher object constructor
*
* @param host - remote host (useful for debug only)
*/
dbuswatcher::dbuswatcher(const char* host) : m_bus(sdbusplus::bus::open_system(
                host))
    , m_running(true)
{
    m_interfacesAddedMatch = std::make_shared<sdbusplus::bus::match::match>(
                                 m_bus,
                                 "type='signal',interface='org.freedesktop.DBus.ObjectManager',"
                                 "member='InterfacesAdded',path='" SENSORS_FOLDER "'",
                                 std::bind(&dbuswatcher::onInterfacesAdded, this, std::placeholders::_1));

    m_powerStateMatch = std::make_shared<sdbusplus::bus::match::match>(
                            m_bus,
                            "type='signal',interface='" DBUS_PROPETIES_IFACE "',"
                            "member='PropertiesChanged',path='" POWER_STATE_PATH "'",
                            std::bind(&dbuswatcher::onPowerStateChanged, this, std::placeholders::_1));
}
/**
 * @brief Get current host power state
 */
void dbuswatcher::updatePowerState(void)
{
    TRACE("Updating power state of host ...\n");

    auto m = m_bus.new_method_call(POWER_STATE_IFACE,
                                   POWER_STATE_PATH,
                                   DBUS_PROPETIES_IFACE,
                                   "Get");
    m.append(POWER_STATE_IFACE, "state");
    auto r = m_bus.call(m);
    if (r.is_method_error())
    {
        TRACE_ERROR("Call Get('" POWER_STATE_IFACE "', 'state') failed!\n");
        hostPowerState = -1;
        return;
    }

    sdbusplus::message::variant<int32_t> d;
    r.read(d);

    hostPowerState = d.get<int32_t>();
    TRACE("Host power state is %d\n", hostPowerState);
}
/**
 * @brief Update list of available sensors
 */
void dbuswatcher::updateSensors(void)
{
    TRACE("Updating sensros ...\n");

    auto m = m_bus.new_method_call(OBJECT_MAPPER_IFACE,
                                   OBJECT_MAPPER_PATH,
                                   OBJECT_MAPPER_IFACE,
                                   "GetSubTree");
    m.append(SENSORS_FOLDER, 5, std::vector<std::string>());
    auto r = m_bus.call(m);
    if (r.is_method_error())
    {
        TRACE_ERROR("Call GetSubTree() failed!\n");
        return;
    }

    std::map<std::string, std::map<std::string, std::vector<std::string> > > d;
    std::string name, type;

    r.read(d);

    // Disable active sensors witch not is present in answer
    for (auto it = m_sensorsMatches.begin(); it != m_sensorsMatches.end();)
    {
        if (0 == d.count(it->first))
        {
            if (splitObjectPath(it->first, name, type))
            {
                sensors_arr_t& arr = getSensorsArr(type);
                auto sit = std::lower_bound(arr.begin(), arr.end(), name);
                if (sit != arr.end() && name == sit->name)
                {
                    auto prev = sensor_t::E_DISABLED;
                    std::swap(sit->state, prev);
                    if (prev != sensor_t::E_DISABLED)
                    {
                        sensorChangeState(&(*sit), type, prev);
                    }
                }
            }

            it = m_sensorsMatches.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Get actual values for sensors from answer
    for (auto sit = d.begin(); sit != d.end(); ++sit)
    {
        for (auto dit = sit->second.begin(); dit != sit->second.end(); ++dit)
        {
            getSensorValues(dit->first, sit->first);
        }
    }
}
/**
 * @brief Main loop
 */
void dbuswatcher::run(void)
{
    if (m_running)
    {
        updatePowerState();
    }

    if (m_running)
    {
        updateSensors();
    }

    TRACE("Start watching...\n");

    while (m_running)
    {
        m_bus.process_discard();
        if (m_running)
        {
            m_bus.wait(100000 /*microseconds*/);
        }
    }
}
/**
 * @brief Called when current value of sensor changed
 *
 * @param sensor - pointer to sensor data
 * @param prev   - previous sensor value
 */
void dbuswatcher::sensorChangeValue(sensor_t* sensor, int prev)
{
    TRACE("Sensor '%s' [%d] change value: %d -> %d\n",
          sensor->name.c_str(), sensor->state, prev, sensor->currentValue);
}
/**
 * @brief Called when state of sensor changed
 *
 * @param sensor - pointer to sensor data
 * @param prev   - previous state of sensor
 */
void dbuswatcher::sensorChangeState(sensor_t* sensor, const std::string& type,
                                    sensor_t::state_t prev)
{
    if (prev == sensor_t::E_DISABLED)
    {
        TRACE("Activate %s sensor '%s': {%d, %d, %d, %d, %d} state: %d\n",
              type.c_str(),
              sensor->name.c_str(), sensor->currentValue,
              sensor->warningLow,   sensor->warningHigh,
              sensor->criticalLow,  sensor->criticalHigh,
              sensor->state);
    }
    else
    {
        TRACE("%s sensor '%s' change state: %d -> %d\n",
              type.c_str(), sensor->name.c_str(), prev,  sensor->state);
    }
}
/**
 * @brief Called when host power state changed
 *
 * @param prev - previous state
 */
void dbuswatcher::powerStateChanged(int prev)
{
    TRACE("Host power state change value: %d -> %d\n",
          prev, hostPowerState);
}
/**
 * @brief Get from DBus object path folder and sensor name
 *
 * @param path - [in]  DBus object path
 * @param name - [out] sensor name
 * @param type - [out] sensor type (folder)
 *
 * @return     - true if success
 */
bool dbuswatcher::splitObjectPath(const std::string& path,
                                  std::string& name, std::string& type) const
{
    size_t n = path.rfind('/');
    if (n != std::string::npos)
    {
        size_t f = path.rfind('/', n - 1);
        if (f != std::string::npos)
        {
            name = path.substr(n + 1);
            type = path.substr(f + 1, n - f - 1);
            return true;
        }
        else
        {
            TRACE_ERROR("splitObjectPath('%s') no folder found!\n", path.c_str());
        }
    }
    else
    {
        TRACE_ERROR("splitObjectPath('%s') no name found!\n", path.c_str());
    }

    return false;
}
/**
 * @brief Get sensors properties from DBus
 *
 * @param object - DBus object
 * @param path   - sensor path
 */
void dbuswatcher::getSensorValues(const std::string& object,
                                  const std::string& path)
{
    std::string name, type;
    if (!splitObjectPath(path, name, type))
    {
        return;
    }

    sensors_arr_t& arr = getSensorsArr(type);
    auto it = std::lower_bound(arr.begin(), arr.end(), name);
    if (it != arr.end() && it->name == name)
    {
        auto m = m_bus.new_method_call(object.c_str(),
                                       path.c_str(),
                                       DBUS_PROPETIES_IFACE,
                                       "GetAll");
        m.append("");
        auto r = m_bus.call(m);

        if (m.is_method_error())
        {
            TRACE_ERROR("GetAll() for sensor '%s' failed!\n", path.c_str());
            return;
        }

        std::map<std::string, sdbusplus::message::variant<std::string, int64_t, bool> >
        d;
        r.read(d);

        int power = getSensorPower(type);
        if (d.count("Scale"))
        {
            power -= d["Scale"].get<int64_t>();
        }

        int scale = static_cast<int>(powf(10.f, power));

        if (d.count("Value"))
        {
            it->currentValue = SCALE_AND_ROUND(d["Value"].get<int64_t>(), scale);//scale * d["Value"].get<int64_t>();
        }

        if (d.count("CriticalHigh"))
        {
            it->criticalHigh = scale * d["CriticalHigh"].get<int64_t>();
        }

        if (d.count("CriticalLow"))
        {
            it->criticalLow = scale * d["CriticalLow"].get<int64_t>();
        }

        if (d.count("WarningHigh"))
        {
            it->warningHigh = scale * d["WarningHigh"].get<int64_t>();
        }

        if (d.count("WarningLow"))
        {
            it->warningLow = scale * d["WarningLow"].get<int64_t>();
        }

        m_sensorsMatches[path] = std::make_shared<match_t>(m_bus,
                                 std::string(
                                     "type='signal',interface='org.freedesktop.DBus.Properties',"
                                     "member='PropertiesChanged',path='") + path + std::string("'"),
                                 std::bind(&dbuswatcher::onPropertiesChanged, this, &(*it), type, scale,
                                           std::placeholders::_1));

        auto prev = it->enable(true);
        if (prev != it->state)
        {
            sensorChangeState(&(*it), type, prev);
        }
    }
    else
    {
        TRACE_ERROR("Unknown %s sensor '%s' found!\n", type.c_str(), name.c_str());
    }
}
/**
 * @brief Called when sensor interface added to DBus
 *
 * @param m - DBus message reference
 */
void dbuswatcher::onInterfacesAdded(sdbusplus::message::message& m)
{
    sdbusplus::message::object_path path;
    std::map<std::string, std::map<std::string, sdbusplus::message::variant<int64_t, bool, std::string> > >
    data;

    m.read(path, data);

    std::string name, type;
    if (splitObjectPath(path.str, name, type))
    {
        sensors_arr_t& arr = getSensorsArr(type);
        auto it = std::lower_bound(arr.begin(), arr.end(), name);
        if (it != arr.end() && it->name == name)
        {
            int power = getSensorPower(type);

            if (data.count(SENSOR_VALUE) && data[SENSOR_VALUE].count("Scale"))
            {
                power -= data[SENSOR_VALUE]["Scale"].get<int64_t>();
            }

            int scale = static_cast<int>(powf(10.f, power));

            if (data.count(SENSOR_VALUE) && data[SENSOR_VALUE].count("Value"))
            {
                it->currentValue = SCALE_AND_ROUND(data[SENSOR_VALUE]["Value"].get<int64_t>(), scale);//scale * data[SENSOR_VALUE]["Value"].get<int64_t>();
            }

            if (data.count(SENSOR_CRITICAL))
            {
                if (data[SENSOR_CRITICAL].count("CriticalHigh"))
                {
                    it->criticalHigh = scale *
                                       data[SENSOR_CRITICAL]["CriticalHigh"].get<int64_t>();
                }

                if (data[SENSOR_CRITICAL].count("CriticalLow"))
                {
                    it->criticalLow = scale * data[SENSOR_CRITICAL]["CriticalLow"].get<int64_t>();
                }
            }

            if (data.count(SENSOR_WARNING))
            {
                if (data[SENSOR_WARNING].count("WarningHigh"))
                {
                    it->warningHigh = scale * data[SENSOR_WARNING]["WarningHigh"].get<int64_t>();
                }

                if (data[SENSOR_WARNING].count("WarningLow"))
                {
                    it->warningLow = scale * data[SENSOR_WARNING]["WarningLow"].get<int64_t>();
                }
            }

            m_sensorsMatches[path.str] = std::make_shared<match_t>(m_bus,
                                         std::string(
                                             "type='signal',interface='org.freedesktop.DBus.Properties',"
                                             "member='PropertiesChanged',path='") + path.str + std::string("'"),
                                         std::bind(&dbuswatcher::onPropertiesChanged, this, &(*it), type, scale,
                                                   std::placeholders::_1));

            auto prev = it->enable(true);
            if (prev != it->state)
            {
                sensorChangeState(&(*it), type, prev);
            }
        }
        else
        {
            TRACE_ERROR("Unknown sensor found '%s'!\n", std::string(path).c_str());
        }
    }
    else
    {
        TRACE_ERROR("Invalid path received '%s'!\n", std::string(path).c_str());
    }
}
/**
 * @brief Called when received DBus signal about changed sensor values
 *
 * @param s     - pointer to sensor data
 * @param type  - type of sensor
 * @param scale - result scale
 * @param m     - DBus message reference
 */
void dbuswatcher::onPropertiesChanged(sensor_t* s, const std::string& type,
                                      int scale, sdbusplus::message::message& m)
{
    std::string iface;
    std::map<std::string, sdbusplus::message::variant<int64_t, bool, std::string> >
    data;
    std::vector<std::string> v;

    m.read(iface, data, v);

    if (iface.compare(SENSOR_VALUE) == 0 && data.count("Value"))
    {
        int value = SCALE_AND_ROUND(data["Value"].get<int64_t>(), scale);
        std::swap(s->currentValue, value);
        sensorChangeValue(s, value);
    }
    else if (iface.compare(SENSOR_WARNING) == 0)
    {
        if (data.count("WarningAlarmHigh"))
        {
            auto value = (data["WarningAlarmHigh"].get<bool>()
                          ? sensor_t::E_WARNING_HIGH
                          : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
        else if (data.count("WarningAlarmLow"))
        {
            auto value = (data["WarningAlarmLow"].get<bool>()
                          ? sensor_t::E_WARNING_LOW
                          : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
    }
    else if (iface.compare(SENSOR_CRITICAL) == 0)
    {
        if (data.count("CriticalAlarmHigh"))
        {
            auto value = (data["CriticalAlarmHigh"].get<bool>()
                          ? sensor_t::E_CRITICAL_HIGH
                          : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
        else if (data.count("CriticalAlarmLow"))
        {
            auto value = (data["CriticalAlarmLow"].get<bool>()
                          ? sensor_t::E_CRITICAL_LOW
                          : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
    }
    else
        TRACE("Skip changed property '%s' for sensor '%s'\n",
              iface.c_str(), s->name.c_str());
}
/**
 * @brief Called when received DBus signal about changed host power state
 *
 * @param m - DBus message reference
 */
void dbuswatcher::onPowerStateChanged(sdbusplus::message::message& m)
{
    std::string iface;
    std::map<std::string, sdbusplus::message::variant<int32_t> > data;
    std::vector<std::string> v;

    m.read(iface, data, v);

    if (data.count("state"))
    {
        int value = data["state"].get<int32_t>();
        std::swap(hostPowerState, value);
        powerStateChanged(value);

        // When host going down some sensors is lost.
        if (hostPowerState == 0 && m_running)
        {
            updateSensors();
        }
    }
}
