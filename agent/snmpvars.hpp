/**
 * @brief netsnmp_variable_list C++ wrapper.
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

namespace phosphor
{
namespace snmp
{
namespace agent
{

/**
 * @brief SNMP representation of boolean type.
 */
enum class TruthValue : int
{
    True = 1,
    False = 2,
};

#define SNMPBOOL(value)                                                        \
    static_cast<int>(value ? TruthValue::True : TruthValue::False)

struct VariableList
{
    /**
     * @brief Fill snmp field with string value.
     */
    static void set(netsnmp_variable_list* var, const std::string& value)
    {
        snmp_set_var_typed_value(var, ASN_OCTET_STR, value.c_str(),
                                 value.length());
    }

    /**
     * @brief Fill snmp field with boolean value.
     */
    static void set(netsnmp_variable_list* var, bool value)
    {
        snmp_set_var_typed_integer(var, ASN_INTEGER, SNMPBOOL(value));
    }

    /**
     * @brief Fill snmp field with integral value.
     */
    template <typename T> static void set(netsnmp_variable_list* var, T&& value)
    {
        snmp_set_var_typed_integer(var, ASN_INTEGER, value);
    }

    /**
     * @brief Add string as field into snmp variables list.
     *
     * @param vars - Pointer to snmp variables list
     * @param field_oid - field OID
     * @param field_oid_len - length of field OID
     * @param field_value - field value
     */
    static void add(netsnmp_variable_list* vars, const oid* field_oid,
                    size_t field_oid_len, const std::string& field_value)
    {
        snmp_varlist_add_variable(
            &vars, field_oid, field_oid_len, ASN_OCTET_STR,
            reinterpret_cast<const u_char*>(field_value.data()),
            field_value.length());
    }

    /**
     * @brief Add boolean as field into snmp variables list.
     *
     * @param vars - Pointer to snmp variables list
     * @param field_oid - field OID
     * @param field_oid_len - length of field OID
     * @param field_value - field value
     */
    static void add(netsnmp_variable_list* vars, const oid* field_oid,
                    size_t field_oid_len, bool field_value)
    {
        auto v = SNMPBOOL(field_value);
        snmp_varlist_add_variable(&vars, field_oid, field_oid_len, ASN_INTEGER,
                                  &v, sizeof(v));
    }
    /**
     * @brief Add value of integral type as field into snmp variables list.
     *
     * @param vars - Pointer to snmp variables list
     * @param field_oid - field OID
     * @param field_oid_len - length of field OID
     * @param field_value - field value
     */
    template <typename T>
    static void add(netsnmp_variable_list* vars, const oid* field_oid,
                    size_t field_oid_len, T&& field_value)
    {
        snmp_varlist_add_variable(&vars, field_oid, field_oid_len, ASN_INTEGER,
                                  reinterpret_cast<const u_char*>(&field_value),
                                  sizeof(field_value));
    }
};

} // namespace agent
} // namespace snmp
} // namespace phosphor
