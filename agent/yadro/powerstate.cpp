/**
 * @brief YADRO host power state implementation.
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
 */
#include "data/scalar.hpp"
#include "yadro/yadro_oid.hpp"
#include "tracing.hpp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "snmptrap.hpp"

namespace yadro
{
namespace host
{
namespace power
{
namespace state
{

static constexpr oid state_oid[] = YADRO_OID(1, 1);
static constexpr oid state_notify_oid[] = YADRO_OID(0, 1);

struct State : public phosphor::snmp::data::Scalar<int32_t>
{
    static constexpr auto IFACE = "org.openbmc.control.Power";
    static constexpr auto PATH = "/org/openbmc/control/power0";

    State() :
        phosphor::snmp::data::Scalar<int32_t>(IFACE, PATH, IFACE, "state", -1)
    {
    }

    void setValue(value_t& var)
    {
        int prev = getValue();

        phosphor::snmp::data::Scalar<int32_t>::setValue(var);

        if (prev != getValue())
        {
            TRACE_INFO("Host power state changed: %d -> %d\n", prev,
                       getValue());

            snmpagent_send_trap(state_notify_oid, OID_LENGTH(state_notify_oid),
                                state_oid, OID_LENGTH(state_oid), getValue());
        }
    }
};

static State state;

/** @brief Handler for snmp requests */
static int State_snmp_handler(netsnmp_mib_handler* /*handler*/,
                              netsnmp_handler_registration* /*reginfo*/,
                              netsnmp_agent_request_info* reqinfo,
                              netsnmp_request_info* requests)
{
    DEBUGMSGTL(("yadro:handle",
                "Processing request (%d) for yadroHostPowerState\n",
                reqinfo->mode));

    switch (reqinfo->mode)
    {
        case MODE_GET:
            for (netsnmp_request_info* request = requests; request;
                 request = request->next)
            {
                snmp_set_var_typed_integer(request->requestvb, ASN_INTEGER,
                                           state.getValue());
            }
            break;
    }

    return SNMP_ERR_NOERROR;
}

void init()
{
    DEBUGMSGTL(("yadro:init", "Initialize yadroHostPowerState\n"));

    state.update();

    netsnmp_register_read_only_instance(netsnmp_create_handler_registration(
        "yadroHostPowerState", State_snmp_handler, state_oid,
        OID_LENGTH(state_oid), HANDLER_CAN_RONLY));
}
void destroy()
{
    DEBUGMSGTL(("yadro:shutdown", "destroy yadroHostPowerState\n"));
    unregister_mib(const_cast<oid*>(state_oid), OID_LENGTH(state_oid));
}

} // namespace state
} // namespace power
} // namespace host
} // namespace yadro
