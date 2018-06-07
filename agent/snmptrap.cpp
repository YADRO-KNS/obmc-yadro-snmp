/**
 * @brief SNMP Traps implementation
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

#include "tracing.hpp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/*
 * In the notification, we have to assign our notification OID to
 * the snmpTrapOID.0 object. Here is it's defintion.
 */
constexpr oid objid_snmptrap_oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

void snmpagent_send_trap(const oid* trap_oid, size_t trap_oid_len,
                         const oid* field_oid, size_t field_oid_len,
                         int field_value)
{
    DEBUGMSGTL(("yadro:notification", "send_trap OID: "));
    DEBUGMSGOID(("yadro:notification", trap_oid, trap_oid_len));
    DEBUGMSG(("yadro:notification", ", Value OID: "));
    DEBUGMSGOID(("yadro:notification", field_oid, field_oid_len));
    DEBUGMSG(("yadro:notification", ", Value: %d\n", field_value));

    netsnmp_variable_list* vars = nullptr;

    // add in the trap definition object
    snmp_varlist_add_variable(&vars,
                              /* the snmpTrapOID.0 variable */
                              objid_snmptrap_oid,
                              OID_LENGTH(objid_snmptrap_oid),
                              /* value type is an OID */
                              ASN_OBJECT_ID,
                              /* value contents is our notification OID */
                              reinterpret_cast<const u_char*>(trap_oid),
                              /* size of notification OID in bytes */
                              trap_oid_len * sizeof(oid));

    // add additional objects defined as part of the trap
    snmp_varlist_add_variable(&vars, field_oid, field_oid_len, ASN_INTEGER,
                              reinterpret_cast<u_char*>(&field_value),
                              sizeof(field_value));

    send_v2trap(vars);
    snmp_free_varbind(vars);
}
