/**
 * @brief YADRO sensors tables implementation.
 *
 * This file is part of yadro-snmp project.
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
 */

#include "tracing.hpp"
#include "data/table.hpp"
#include "data/table/item.hpp"
#include "yadro/yadro_oid.hpp"
#include "snmptrap.hpp"

#include <cmath>

namespace yadro
{
namespace sensors
{
/**
 * @brief Sensor implementation.
 */
struct Sensor
    : public phosphor::snmp::data::table::Item<double, double, bool, double,
                                               bool, double, bool, double, bool>
{
    /**
     * @brief State codes like in MIB file.
     */
    enum state_t
    {
        E_DISABLED = 0,
        E_NORMAL,
        E_WARNING_LOW,
        E_WARNING_HIGH,
        E_CRITICAL_LOW,
        E_CRITICAL_HIGH,
    };

    // SNMP table columns
    enum Columns
    {
        COLUMN_YADROSENSOR_NAME = 1,
        COLUMN_YADROSENSOR_VALUE,
        COLUMN_YADROSENSOR_WARNLOW,
        COLUMN_YADROSENSOR_WARNHIGH,
        COLUMN_YADROSENSOR_CRITLOW,
        COLUMN_YADROSENSOR_CRITHIGH,
        COLUMN_YADROSENSOR_STATE,
    };

    // Indexes of fields in tuple
    enum Fields
    {
        FIELD_SENSOR_VALUE = 0,
        FIELD_SENSOR_WARNLOW,
        FIELD_SENSOR_WARNLOW_ALARM,
        FIELD_SENSOR_WARNHI,
        FIELD_SENSOR_WARNHI_ALARM,
        FIELD_SENSOR_CRITLOW,
        FIELD_SENSOR_CRITLOW_ALARM,
        FIELD_SENSOR_CRITHI,
        FIELD_SENSOR_CRITHI_ALARM,
    };

    // Sensor types (first letter in sensors folder name)
    enum Types
    {
        ST_TEMPERATURE = 't', // temperature
        ST_VOLTAGE = 'v',     // voltage
        ST_TACHOMETER = 'f',  // fan_tach
        ST_CURRENT = 'c',     // current
        ST_POWER = 'p',       // power
    };

    /**
     * @brief Object contructor.
     */
    Sensor(const std::string& folder, const std::string& name) :
        phosphor::snmp::data::table::Item<double, double, bool, double, bool,
                                          double, bool, double, bool>(
            folder, name,
            .0,        // Value
            .0, false, // WarningLow
            .0, false, // WarningHigh
            .0, false, // CriticalLow
            .0, false) // CriticalHigh
    {
        auto n = folder.rfind('/');
        if (n != std::string::npos)
        {
            // Prepare for send traps
            switch (folder[n + 1])
            {
                case ST_TEMPERATURE:
                    _notifyOid.assign(YADRO_OID(0, 2));
                    break;

                case ST_VOLTAGE:
                    _notifyOid.assign(YADRO_OID(0, 3));
                    break;

                case ST_TACHOMETER:
                    _notifyOid.assign(YADRO_OID(0, 4));
                    break;

                case ST_CURRENT:
                    _notifyOid.assign(YADRO_OID(0, 5));
                    break;

                case ST_POWER:
                    _notifyOid.assign(YADRO_OID(0, 6));
                    break;
            }

            // Correct scale power
            // Required for TEXTUAL-CONVENTION in YADRO-MIB.txt
            switch (folder[n + 1])
            {
                case ST_TACHOMETER:
                    _power = 0;
                    break;
            }
        }

        if (!_notifyOid.empty())
        {
            phosphor::snmp::agent::make_oid(
                _stateOid, ".1.3.6.1.4.1.49769.1.%lu.1.7.\"%s\"",
                _notifyOid.back(), name.c_str());
        }
    }

    /**
     * @brief Update fields with new values recieved from DBus.
     */
    void setFields(const fields_map_t& fields) override
    {
        auto prevValue = getValue<FIELD_SENSOR_VALUE>();
        auto prevState = getState();

        setField<FIELD_SENSOR_VALUE>(fields, "Value");
        setField<FIELD_SENSOR_WARNLOW>(fields, "WarningLow");
        setField<FIELD_SENSOR_WARNLOW_ALARM>(fields, "WarningAlarmLow");
        setField<FIELD_SENSOR_WARNHI>(fields, "WarningHigh");
        setField<FIELD_SENSOR_WARNHI_ALARM>(fields, "WarningAlarmHigh");
        setField<FIELD_SENSOR_CRITLOW>(fields, "CriticalLow");
        setField<FIELD_SENSOR_CRITLOW_ALARM>(fields, "CriticalAlarmLow");
        setField<FIELD_SENSOR_CRITHI>(fields, "CriticalHigh");
        setField<FIELD_SENSOR_CRITHI_ALARM>(fields, "CriticalAlarmHigh");

        auto lastState = getState();

        if (prevValue != getValue<FIELD_SENSOR_VALUE>() ||
            prevState != lastState)
        {
            TRACE_DEBUG("Sensor '%s' changed: %d -> %d, state: %d -> %d\n",
                        name.c_str(), prevValue, getValue<FIELD_SENSOR_VALUE>(),
                        prevState, lastState);
        }

        if (prevState != lastState)
        {
            send_notify(lastState);
        }
    }

    /**
     * @brief Called when sensor added to table.
     */
    void onCreate() override
    {
        TRACE_DEBUG("Sensor '%s' added, state=%d\n", name.c_str(), getState());
        send_notify(E_NORMAL);
    }

    /**
     * @brief Called when sensor removed from table.
     */
    void onDestroy() override
    {
        TRACE_DEBUG("Sensor '%s' removed, state=%d\n", name.c_str(),
                    getState());
        send_notify(E_DISABLED);
    }

    /**
     * @brief Send snmptrap about changed state.
     *
     * @param state - state value for sent with trap.
     */
    void send_notify(state_t state)
    {
        if (!_notifyOid.empty() && !_stateOid.empty())
        {
            phosphor::snmp::agent::Trap trap(_notifyOid);
            trap.add_field(_stateOid.data(), _stateOid.size(), state);
            trap.send();
        }
        else
        {
            TRACE_ERROR("Notify is unsupported for sensor '%s'\n",
                        name.c_str());
        }
    }

    /**
     * @brief Get current state.
     */
    state_t getState() const
    {
        if (std::get<FIELD_SENSOR_CRITHI_ALARM>(data))
        {
            return E_CRITICAL_HIGH;
        }
        else if (std::get<FIELD_SENSOR_WARNHI_ALARM>(data))
        {
            return E_WARNING_HIGH;
        }
        else if (std::get<FIELD_SENSOR_WARNLOW_ALARM>(data))
        {
            return E_WARNING_LOW;
        }
        else if (std::get<FIELD_SENSOR_CRITLOW_ALARM>(data))
        {
            return E_CRITICAL_LOW;
        }

        return E_NORMAL;
    }

    /**
     * @brief snmp request handler.
     */
    void get_snmp_reply(netsnmp_agent_request_info* reqinfo,
                        netsnmp_request_info* request) const override
    {
        using namespace phosphor::snmp::agent;

        netsnmp_table_request_info* tinfo = netsnmp_extract_table_info(request);

        switch (tinfo->colnum)
        {
            case COLUMN_YADROSENSOR_VALUE:
                VariableList::set(request->requestvb,
                                  getValue<FIELD_SENSOR_VALUE>());
                break;

            case COLUMN_YADROSENSOR_WARNLOW:
                VariableList::set(request->requestvb,
                                  getValue<FIELD_SENSOR_WARNLOW>());
                break;

            case COLUMN_YADROSENSOR_WARNHIGH:
                VariableList::set(request->requestvb,
                                  getValue<FIELD_SENSOR_WARNHI>());
                break;

            case COLUMN_YADROSENSOR_CRITLOW:
                VariableList::set(request->requestvb,
                                  getValue<FIELD_SENSOR_CRITLOW>());
                break;

            case COLUMN_YADROSENSOR_CRITHIGH:
                VariableList::set(request->requestvb,
                                  getValue<FIELD_SENSOR_CRITHI>());
                break;

            case COLUMN_YADROSENSOR_STATE:
                VariableList::set(request->requestvb, getState());
                break;

            default:
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                break;
        }
    }

    /**
     * @brief Scale and round sensors value.
     */
    template <size_t Idx> int getValue() const
    {
        return static_cast<int>(
            std::round(std::get<Idx>(data) * powf(10.f, _power)));
    }

    std::vector<oid> _notifyOid;
    std::vector<oid> _stateOid;
    int _power = 3;
};

struct SensorsTable : public phosphor::snmp::data::Table<Sensor>
{
    using OID = std::vector<oid>;

    SensorsTable(const std::string& folder, const std::string& tableName,
                 const OID& tableOID) :
        phosphor::snmp::data::Table<Sensor>(
            "/xyz/openbmc_project/sensors", folder,
            {
                "xyz.openbmc_project.Sensor.Value",
                "xyz.openbmc_project.Sensor.Threshold.Warning",
                "xyz.openbmc_project.Sensor.Threshold.Critical",
                "xyz.openbmc_project.Sensor.Threshold.Fatal",
            }),
        tableName(tableName), tableOID(tableOID)

    {
    }

    std::string tableName;
    OID tableOID;
};

static std::array<SensorsTable, 5> sensors = {
    SensorsTable{"temperature", "yadorTempSensorsTable", YADRO_OID(1, 2)},
    SensorsTable{"voltage", "yadroVoltSensorsTable", YADRO_OID(1, 3)},
    SensorsTable{"fan_tach", "yadroTachSensorsTable", YADRO_OID(1, 4)},
    SensorsTable{"current", "yadroCurrSensorsTable", YADRO_OID(1, 5)},
    SensorsTable{"power", "yadroPowerSensorsTable", YADRO_OID(1, 6)},
};

/**
 * @brief Update all sensors.
 */
void update()
{
    for (auto& s : sensors)
    {
        s.update();
    }
}

/**
 * @brief Initialize sensors.
 */
void init()
{
    DEBUGMSGTL(("yadro:init", "Initialize yadroSensors\n"));

    for (auto& s : sensors)
    {
        s.init_mib(s.tableName.c_str(), s.tableOID.data(), s.tableOID.size(),
                   Sensor::COLUMN_YADROSENSOR_VALUE,
                   Sensor::COLUMN_YADROSENSOR_STATE);
        s.update();
    }
}

/**
 * @brief Deinitialize sensors.
 */
void destroy()
{
    DEBUGMSGTL(("yadro:shutdown", "Destroy yadroSensors\n"));

    for (auto& s : sensors)
    {
        unregister_mib(s.tableOID.data(), s.tableOID.size());
    }
}

} // namespace sensors
} // namespace yadro
