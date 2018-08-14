/**
 * @brief SNMP Traps implementation
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
#pragma once

#include <net-snmp/net-snmp-includes.h>
#include <array>
#include "snmp_oid.hpp"
#include "snmpvars.hpp"

namespace phosphor
{
namespace snmp
{
namespace agent
{

namespace details
{

/**
 * @brief unique_ptr functor to release an variable list reference.
 */
struct VariableListDeleter
{
    void operator()(netsnmp_variable_list* ptr) const
    {
        deleter(ptr);
    }

    decltype(&snmp_free_varbind) deleter = snmp_free_varbind;
};

/**
 * @brief Alias 'VariableList' to a unique_ptr type for auto-release.
 */
using VariableList =
    std::unique_ptr<netsnmp_variable_list, VariableListDeleter>;

} // namespace details

/*
 * In the notification, we have to assign our notification OID to
 * the snmpTrapOID.0 object. Here is it's defintion.
 */
constexpr std::array<oid, 11> SNMPTRAP_OID = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

class Trap
{
  public:
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    Trap() = delete;
    Trap(const Trap&) = delete;
    Trap& operator=(const Trap&) = delete;
    Trap(Trap&&) = default;
    Trap& operator=(Trap&&) = default;
    ~Trap() = default;

    explicit Trap(const OID& trap_oid)
    {
        create_variable_list(trap_oid.data(), trap_oid.size());
    }

    Trap(const oid* trap_oid, size_t trap_oid_len)
    {
        create_variable_list(trap_oid, trap_oid_len);
    }

    template <typename T> void add_field(const OID& field_oid, T&& field_value)
    {
        add_field(field_oid.data(), field_oid.size(),
                  std::forward<T>(field_value));
    }

    void add_field(const oid* field_oid, size_t field_oid_len,
                   const std::string field_value)
    {
        DEBUGMSGTL(("snmpagent:trap", "  Field OID: "));
        DEBUGMSGOID(("snmpagent:trap", field_oid, field_oid_len));
        DEBUGMSG(
            ("snmpagent:trap", ", Value: STRING(%s)\n", field_value.c_str()));

        VariableList::add(_vars.get(), field_oid, field_oid_len, field_value);
    }

    void add_field(const oid* field_oid, size_t field_oid_len, bool field_value)
    {
        DEBUGMSGTL(("snmpagent:trap", "  Field OID: "));
        DEBUGMSGOID(("snmpagent:trap", field_oid, field_oid_len));
        DEBUGMSG(("snmpagent:trap", ", Value: BOOLEAN(%s)\n",
                  field_value ? "True" : "False"));

        VariableList::add(_vars.get(), field_oid, field_oid_len, field_value);
    }

    template <typename T>
    void add_field(const oid* field_oid, size_t field_oid_len, T&& field_value)
    {
        DEBUGMSGTL(("snmpagent:trap", "  Field OID: "));
        DEBUGMSGOID(("snmpagent:trap", field_oid, field_oid_len));
        DEBUGMSG(("snmpagent:trap", ", Value: INTEGER(%d)\n", field_value));

        VariableList::add(_vars.get(), field_oid, field_oid_len,
                          std::forward<T>(field_value));
    }

    void send() const
    {
        DEBUGMSGTL(("snmpagent:trap", "send trap\n"));
        send_v2trap(_vars.get());
    }

  protected:
    void create_variable_list(const oid* trap_oid, size_t trap_oid_len)
    {
        DEBUGMSGTL(("snmpagent:trap", "Trap OID: "));
        DEBUGMSGOID(("snmpagent:trap", trap_oid, trap_oid_len));
        DEBUGMSG(("snmpagent:trap", "\n"));

        netsnmp_variable_list* vars = nullptr;

        // add in the trap definition object
        snmp_varlist_add_variable(&vars,
                                  /* the snmpTrapOID.0 variable */
                                  SNMPTRAP_OID.data(), SNMPTRAP_OID.size(),
                                  /* value type is an OID */
                                  ASN_OBJECT_ID,
                                  /* value contents is our notification OID */
                                  reinterpret_cast<const u_char*>(trap_oid),
                                  /* size of notification OID in bytes */
                                  trap_oid_len * sizeof(oid));
        _vars.reset(vars);
    }

    details::VariableList _vars;
};

} // namespace agent
} // namespace snmp
} // namespace phosphor
