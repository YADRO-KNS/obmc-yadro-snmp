/**
 * @brief YADRO software table implementation.
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
#include "data/enums.hpp"
#include "yadro/yadro_oid.hpp"
#include "snmpvars.hpp"

#define INVALID_ENUM_VALUE 0xFF

namespace yadro
{
namespace software
{

/**
 * @brief DBus enum to byte converters.
 */
const phosphor::snmp::data::DBusEnum<uint8_t>
    purpose = {"xyz.openbmc_project.Software.Version.VersionPurpose",
               {
                   {"Unknown", 0},
                   {"Other", 1},
                   {"System", 2},
                   {"BMC", 3},
                   {"Host", 4},
               },
               INVALID_ENUM_VALUE},

    activation = {"xyz.openbmc_project.Software.Activation.Activations",
                  {
                      {"NotReady", 0},
                      {"Invalid", 1},
                      {"Ready", 2},
                      {"Activating", 3},
                      {"Active", 4},
                      {"Failed", 5},
                  },
                  INVALID_ENUM_VALUE};

/**
 * @brief Software item implementation.
 */
struct Software : public phosphor::snmp::data::table::Item<std::string, uint8_t,
                                                           uint8_t, uint8_t>
{
    // Indexes of fields in tuple
    enum Fields
    {
        FIELD_SOFTWARE_VERSION = 0,
        FIELD_SOFTWARE_PURPOSE,
        FIELD_SOFTWARE_ACTIVATION,
        FIELD_SOFTWARE_PRIORITY,
    };

    /**
     * @brief Object constructor.
     */
    Software(const std::string& folder, const std::string& name) :
        phosphor::snmp::data::table::Item<std::string, uint8_t, uint8_t,
                                          uint8_t>(
            folder, name,
            "",                 // Version
            INVALID_ENUM_VALUE, // Purpose
            INVALID_ENUM_VALUE, // Activation
            INVALID_ENUM_VALUE) // Priority
    {
    }

    /**
     * @brief Set field value with DBus enum converter.
     *
     * @tparam Idx - Index of field
     * @param fields - DBus fields map
     * @param field - Name of field in DBus
     * @param enumcvt - Enum converter
     */
    template <size_t Idx>
    void setFieldEnum(const fields_map_t& fields, const char* field,
                      const phosphor::snmp::data::DBusEnum<uint8_t>& enumcvt)
    {
        if (fields.find(field) != fields.end() &&
            std::holds_alternative<std::string>(fields.at(field)))
        {
            std::get<Idx>(data) =
                enumcvt.get(std::get<std::string>(fields.at(field)));
        }
    }

    /**
     * @brief Update fields with new values recieved from DBus.
     */
    void setFields(const fields_map_t& fields) override
    {
        uint8_t prevActivation = std::get<FIELD_SOFTWARE_ACTIVATION>(data),
                prevPriority = std::get<FIELD_SOFTWARE_PRIORITY>(data);

        setField<FIELD_SOFTWARE_VERSION>(fields, "Version");
        setFieldEnum<FIELD_SOFTWARE_PURPOSE>(fields, "Purpose", purpose);
        setFieldEnum<FIELD_SOFTWARE_ACTIVATION>(fields, "Activation",
                                                activation);
        setField<FIELD_SOFTWARE_PRIORITY>(fields, "Priority");

        if (prevActivation != std::get<FIELD_SOFTWARE_ACTIVATION>(data) ||
            prevPriority != std::get<FIELD_SOFTWARE_PRIORITY>(data))
        {
            DEBUGMSGTL(("yadro::software",
                        "Software '%s' version='%s', purpose=%d changed: "
                        "activation %d -> %d, priority %d -> %d\n",
                        name.c_str(),
                        std::get<FIELD_SOFTWARE_VERSION>(data).c_str(),
                        std::get<FIELD_SOFTWARE_PURPOSE>(data), prevActivation,
                        std::get<FIELD_SOFTWARE_ACTIVATION>(data), prevPriority,
                        std::get<FIELD_SOFTWARE_PRIORITY>(data)));
        }
    }

    enum Columns
    {
        COLUMN_YADROSOFTWARE_HASH = 1,
        COLUMN_YADROSOFTWARE_VERSION = 2,
        COLUMN_YADROSOFTWARE_PURPOSE = 3,
        COLUMN_YADROSOFTWARE_ACTIVATION = 4,
        COLUMN_YADROSOFTWARE_PRIORITY = 5,
    };

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
            case COLUMN_YADROSOFTWARE_VERSION:
                VariableList::set(request->requestvb,
                                  std::get<FIELD_SOFTWARE_VERSION>(data));
                break;

            case COLUMN_YADROSOFTWARE_PURPOSE:
                VariableList::set(request->requestvb,
                                  std::get<FIELD_SOFTWARE_PURPOSE>(data));
                break;

            case COLUMN_YADROSOFTWARE_ACTIVATION:
                VariableList::set(request->requestvb,
                                  std::get<FIELD_SOFTWARE_ACTIVATION>(data));
                break;

            case COLUMN_YADROSOFTWARE_PRIORITY:
                VariableList::set(request->requestvb,
                                  std::get<FIELD_SOFTWARE_PRIORITY>(data));
                break;

            default:
                netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHOBJECT);
                break;
        }
    }
};

constexpr oid softwareOid[] = YADRO_OID(5);
constexpr auto SOFTWARE_FOLDER = "/xyz/openbmc_project/software";

static phosphor::snmp::data::Table<Software>
    softwareTable("/xyz/openbmc_project/software", "",
                  {
                      "xyz.openbmc_project.Software.Version",
                      "xyz.openbmc_project.Software.Activation",
                      "xyz.openbmc_project.Software.RedundancyPriority",
                  });

/**
 * @brief Initialize software table
 */
void init()
{
    DEBUGMSGTL(("yadro:init", "Initialize yadroSoftwareTable\n"));

    softwareTable.update();
    softwareTable.init_mib("yadroSoftwareTable", softwareOid,
                           OID_LENGTH(softwareOid),
                           Software::COLUMN_YADROSOFTWARE_VERSION,
                           Software::COLUMN_YADROSOFTWARE_PRIORITY);
}

/**
 * @brief Deinitialize software table
 */
void destroy()
{
    DEBUGMSGTL(("yadro:shutdown", "Deinitialize yadroSoftwareTable\n"));
    unregister_mib(const_cast<oid*>(softwareOid), OID_LENGTH(softwareOid));
}

} // namespace software
} // namespace yadro
