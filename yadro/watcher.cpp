/**
 * @brief DBus objects watcher implementation
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

#include <math.h>      // powf()
#include <tracing.hpp> // TRACE, TRACE_ERROR
#include "watcher.hpp" // dbuswatcher

static constexpr auto SENSORS_FOLDER = "/xyz/openbmc_project/sensors";
static constexpr auto POWER_STATE_IFACE = "org.openbmc.control.Power";
static constexpr auto POWER_STATE_PATH = "/org/openbmc/control/power0";

static constexpr auto SENSOR_VALUE = "xyz.openbmc_project.Sensor.Value";
static constexpr auto SENSOR_WARNING =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
static constexpr auto SENSOR_CRITICAL =
    "xyz.openbmc_project.Sensor.Threshold.Critical";

#define SCALE_AND_ROUND(v, s) static_cast<int>((v + 0.5) / s)

/**
 * @brief dbuswatcher object constructor
 *
 * @param host - remote host (useful for debug only)
 */
dbuswatcher::dbuswatcher(const char* host) : sdbusplus::helper::helper(host)
{
    using namespace sdbusplus::bus::match::rules;

    m_staticMatches.emplace_back(
        m_bus, interfacesAdded(SENSORS_FOLDER),
        std::bind(&dbuswatcher::onSensorsAdded, this, std::placeholders::_1));

    m_staticMatches.emplace_back(m_bus, propertiesChanged(POWER_STATE_PATH),
                                 std::bind(&dbuswatcher::onPowerStateChanged,
                                           this, std::placeholders::_1));

    m_staticMatches.emplace_back(
        m_bus, interfacesRemoved(SENSORS_FOLDER),
        std::bind(&dbuswatcher::onSensorsRemoved, this, std::placeholders::_1));
}
/**
 * @brief Get current host power state
 */
void dbuswatcher::updatePowerState(void)
{
    TRACE("Updating power state of host ...\n");

    hostPowerState = getProperty<int32_t>(POWER_STATE_IFACE, POWER_STATE_PATH,
                                          POWER_STATE_IFACE, "state");

    TRACE("Host power state is %d\n", hostPowerState);
}
/**
 * @brief Update list of available sensors
 */
void dbuswatcher::updateSensors(void)
{
    TRACE("Updating sensros ...\n");

    auto d = getSubTree(SENSORS_FOLDER, SENSOR_VALUE, 5);

    std::string name, type;
    // Disable active sensors wich is not present in answer
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
 * @brief update BMC version
 */
void dbuswatcher::updateBMCVersion()
{
    using objects = std::vector<sdbusplus::message::object_path>;

    auto ep = getProperty<objects>(sdbusplus::helper::OBJECT_MAPPER_IFACE,
                                   "/xyz/openbmc_project/software/active",
                                   "org.openbmc.Association", "endpoints");

    std::string ver;

    if (!ep.empty())
    {
        for (auto p : ep)
        {
            ver = getProperty<std::string>(
                "xyz.openbmc_project.Software.BMC.Updater", p.str,
                "xyz.openbmc_project.Software.Version", "Version");

            if (!ver.empty())
            {
                break;
            }
        }
    }
    else
    {
        // TODO: sometimes ObjectMapper return `endpoints` as array of `strings`
        //       instead `object_pathes`. I do not understand yet - why.
        //       This block of code fix this problem, however should be
        //       removed/rewrited.
        using strings = std::vector<std::string>;
        auto ep = getProperty<strings>(sdbusplus::helper::OBJECT_MAPPER_IFACE,
                                       "/xyz/openbmc_project/software/active",
                                       "org.openbmc.Association", "endpoints");
        for (auto p : ep)
        {
            ver = getProperty<std::string>(
                "xyz.openbmc_project.Software.BMC.Updater", p,
                "xyz.openbmc_project.Software.Version", "Version");
            if (!ver.empty())
            {
                break;
            }
        }
    }

    if (!ver.empty())
    {
        setBMCVersion(ver);
    }
}
/**
 * @brief update Host firmware version
 */
void dbuswatcher::updateHFWVersion(void)
{
    setHFWVersion(getProperty<std::string>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard/opfw",
        "xyz.openbmc_project.Inventory.Decorator.Revision", "Version"));
}
/**
 * @brief Main loop
 */
void dbuswatcher::run(void)
{
    if (isRunning())
    {
        updatePowerState();
    }

    if (isRunning())
    {
        updateSensors();
    }

    if (isRunning())
    {
        updateBMCVersion();
    }

    if (isRunning())
    {
        updateHFWVersion();
    }

    TRACE("Start watching...\n");

    loop();

    TRACE("Stop watching...\n");
}
/**
 * @brief Called when current value of sensor changed
 *
 * @param sensor - pointer to sensor data
 * @param prev   - previous sensor value
 */
void dbuswatcher::sensorChangeValue(sensor_t* sensor, int prev)
{
    TRACE("Sensor '%s' [%d] change value: %d -> %d\n", sensor->name.c_str(),
          sensor->state, prev, sensor->currentValue);
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
              type.c_str(), sensor->name.c_str(), sensor->currentValue,
              sensor->warningLow, sensor->warningHigh, sensor->criticalLow,
              sensor->criticalHigh, sensor->state);
    }
    else
    {
        TRACE("%s sensor '%s' change state: %d -> %d\n", type.c_str(),
              sensor->name.c_str(), prev, sensor->state);
    }
}
/**
 * @brief Called when host power state changed
 *
 * @param prev - previous state
 */
void dbuswatcher::powerStateChanged(int prev)
{
    TRACE("Host power state change value: %d -> %d\n", prev, hostPowerState);
}
/**
 * @brief Called when BMC Version changed.
 *
 * @param prev - previous version value
 */
void dbuswatcher::versionBMCChanged(const std::string& prev)
{
    TRACE("BMC Version changed: '%s' -> '%s'\n", prev.c_str(), versionBMC);
}
/**
 * @brief Called when Host firmware version changed.
 *
 * @param prev - previous version value
 */
void dbuswatcher::versionHFWChanged(const std::string& prev)
{
    TRACE("Host FW Version changed: '%s' -> '%s'\n", prev.c_str(), versionHFW);
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
bool dbuswatcher::splitObjectPath(const std::string& path, std::string& name,
                                  std::string& type) const
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
            TRACE_ERROR("splitObjectPath('%s') no folder found!\n",
                        path.c_str());
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
    using sensors_values_t =
        std::map<std::string, sdbusplus::message::variant<int64_t>>;

    std::string name, type;
    if (!splitObjectPath(path, name, type))
    {
        return;
    }

    sensors_arr_t& arr = getSensorsArr(type);
    auto it = std::lower_bound(arr.begin(), arr.end(), name);
    if (it != arr.end() && it->name == name)
    {
        auto d = callMethodAndRead<sensors_values_t>(
            object, path, sdbusplus::helper::PROPETIES_IFACE, "GetAll", "");

        int power = getSensorPower(type);
        if (d.count("Scale"))
        {
            power -= d["Scale"].get<int64_t>();
        }

        int scale = static_cast<int>(powf(10.f, power));

        if (d.count("Value"))
        {
            it->currentValue =
                SCALE_AND_ROUND(d["Value"].get<int64_t>(), scale);
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

        if (0 == m_sensorsMatches.count(path))
        {
            m_sensorsMatches.emplace(
                std::piecewise_construct, std::forward_as_tuple(path),
                std::forward_as_tuple(
                    m_bus,
                    sdbusplus::bus::match::rules::propertiesChanged(path),
                    std::bind(&dbuswatcher::onPropertiesChanged, this, &(*it),
                              type, scale, std::placeholders::_1)));
        }

        auto prev = it->enable(true);
        if (prev != it->state)
        {
            sensorChangeState(&(*it), type, prev);
        }
    }
    else
    {
        TRACE_ERROR("Unknown %s sensor '%s' found!\n", type.c_str(),
                    name.c_str());
    }
}
/**
 * @brief Called when sensors interface added to DBus
 *
 * @param m - DBus message reference
 */
void dbuswatcher::onSensorsAdded(sdbusplus::message::message& m)
{
    sdbusplus::message::object_path path;
    std::map<std::string,
             std::map<std::string,
                      sdbusplus::message::variant<int64_t, bool, std::string>>>
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
                it->currentValue = SCALE_AND_ROUND(
                    data[SENSOR_VALUE]["Value"].get<int64_t>(), scale);
            }

            if (data.count(SENSOR_CRITICAL))
            {
                if (data[SENSOR_CRITICAL].count("CriticalHigh"))
                {
                    it->criticalHigh =
                        scale *
                        data[SENSOR_CRITICAL]["CriticalHigh"].get<int64_t>();
                }

                if (data[SENSOR_CRITICAL].count("CriticalLow"))
                {
                    it->criticalLow =
                        scale *
                        data[SENSOR_CRITICAL]["CriticalLow"].get<int64_t>();
                }
            }

            if (data.count(SENSOR_WARNING))
            {
                if (data[SENSOR_WARNING].count("WarningHigh"))
                {
                    it->warningHigh =
                        scale *
                        data[SENSOR_WARNING]["WarningHigh"].get<int64_t>();
                }

                if (data[SENSOR_WARNING].count("WarningLow"))
                {
                    it->warningLow =
                        scale *
                        data[SENSOR_WARNING]["WarningLow"].get<int64_t>();
                }
            }

            if (0 == m_sensorsMatches.count(path.str))
            {
                m_sensorsMatches.emplace(
                    std::piecewise_construct, std::forward_as_tuple(path.str),
                    std::forward_as_tuple(
                        m_bus,
                        sdbusplus::bus::match::rules::propertiesChanged(
                            path.str),
                        std::bind(&dbuswatcher::onPropertiesChanged, this,
                                  &(*it), type, scale, std::placeholders::_1)));
            }

            auto prev = it->enable(true);
            if (prev != it->state)
            {
                sensorChangeState(&(*it), type, prev);
            }
        }
        else
        {
            TRACE_ERROR("Unknown sensor found '%s'!\n", path.str.c_str());
        }
    }
    else
    {
        TRACE_ERROR("Invalid path received '%s'!\n", path.str.c_str());
    }
}

/**
 * @brief Called when sensors interface removed from DBus.
 *
 * @param m - DBus message reference
 */
void dbuswatcher::onSensorsRemoved(sdbusplus::message::message& m)
{
    sdbusplus::message::object_path path;
    std::vector<std::string> data;

    m.read(path, data);

    std::string name, type;
    if (splitObjectPath(path.str, name, type))
    {
        sensors_arr_t& arr = getSensorsArr(type);
        auto it = std::lower_bound(arr.begin(), arr.end(), name);
        if (it != arr.end() && it->name == name)
        {
            m_sensorsMatches.erase(path.str);
            auto prev = it->enable(false);
            if (prev != it->state)
            {
                sensorChangeState(&(*it), type, prev);
            }
        }
        else
        {
            TRACE_ERROR("Unknown sensor found '%s'!\n", path.str.c_str());
        }
    }
    else
    {
        TRACE_ERROR("Invalid path received '%s'!\n", path.str.c_str());
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
    std::map<std::string,
             sdbusplus::message::variant<int64_t, bool, std::string>>
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
            auto value =
                (data["WarningAlarmHigh"].get<bool>() ? sensor_t::E_WARNING_HIGH
                                                      : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
        else if (data.count("WarningAlarmLow"))
        {
            auto value =
                (data["WarningAlarmLow"].get<bool>() ? sensor_t::E_WARNING_LOW
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
            auto value =
                (data["CriticalAlarmLow"].get<bool>() ? sensor_t::E_CRITICAL_LOW
                                                      : sensor_t::E_NORMAL);
            std::swap(s->state, value);
            sensorChangeState(s, type, value);
        }
    }
    else
        TRACE("Skip changed property '%s' for sensor '%s'\n", iface.c_str(),
              s->name.c_str());
}
/**
 * @brief Called when received DBus signal about changed host power state
 *
 * @param m - DBus message reference
 */
void dbuswatcher::onPowerStateChanged(sdbusplus::message::message& m)
{
    std::string iface;
    std::map<std::string, sdbusplus::message::variant<int32_t>> data;
    std::vector<std::string> v;

    m.read(iface, data, v);

    if (data.count("state"))
    {
        int value = data["state"].get<int32_t>();
        std::swap(hostPowerState, value);
        powerStateChanged(value);

        // When host going down some sensors is lost.
        if (hostPowerState == 0 && isRunning())
        {
            updateSensors();
        }
    }
}
/**
 * @brief Set BMC Version
 *
 * @param ver - new version
 */
void dbuswatcher::setBMCVersion(const std::string& ver)
{
    if (0 != ver.compare(versionBMC))
    {
        auto len = std::min<size_t>(VERSION_MAX_LEN, ver.size());
        std::string prev(versionBMC);

        memset(versionBMC, 0, sizeof(versionBMC));
        memcpy(versionBMC, ver.data(), len);

        versionBMCChanged(prev);
    }
    else
        TRACE("setBMCVersion: versions ('%s' and '%s') is same.\n", ver.c_str(),
              versionBMC);
}
/**
 * @brief Set Host firmware version
 *
 * @param ver - new version
 */
void dbuswatcher::setHFWVersion(const std::string& ver)
{
    if (0 != ver.compare(versionHFW))
    {
        auto len = std::min<size_t>(VERSION_MAX_LEN, ver.size());
        std::string prev(versionHFW);

        memset(versionHFW, 0, sizeof(versionHFW));
        memcpy(versionHFW, ver.data(), len);

        versionHFWChanged(prev);
    }
}
