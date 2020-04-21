/**
 * @brief YADRO host power state implementation.
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
using OID = phosphor::snmp::agent::OID;

static const OID state_oid = YADRO_OID(1, 1);
static const OID notify_oid = YADRO_OID(0, 1);

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
            TRACE_DEBUG("Host power state changed: %d -> %d\n", prev,
                        getValue());

            phosphor::snmp::agent::Trap trap(notify_oid);
            trap.add_field(state_oid, getValue());
            trap.send();
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
                phosphor::snmp::agent::VariableList::set(request->requestvb,
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
        "yadroHostPowerState", State_snmp_handler, state_oid.data(),
        state_oid.size(), HANDLER_CAN_RONLY));
}
void destroy()
{
    DEBUGMSGTL(("yadro:shutdown", "destroy yadroHostPowerState\n"));
    unregister_mib(const_cast<oid*>(state_oid.data()), state_oid.size());
}

} // namespace state
} // namespace power
} // namespace host
} // namespace yadro
