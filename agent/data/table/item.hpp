/**
 * @brief MIB tables item definition.
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

#include "sdbusplus/helper.hpp"

namespace phosphor
{
namespace snmp
{
namespace data
{
namespace table
{

/**
 * @brief MIB Table row implementation.
 */
template <typename... T> struct Item
{
    /*
     * The `std::variant` allows to keep duplicates of types,
     * but `std::get<>()` and `std::holds_alternative<>()` is ill-formed
     * in this case.
     *
     * So we can't use here:
     *   using variant_t = std::variant<T...>;
     * and we should specify all possible types.
     */
    using variant_t = std::variant<int64_t, std::string, bool, uint8_t, double>;

    using values_t = std::tuple<T...>;
    using fields_map_t = std::map<std::string, variant_t>;

    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    Item() = delete;
    Item(const Item&) = delete;
    Item& operator=(const Item&) = delete;
    Item(Item&&) = default;
    Item& operator=(Item&&) = default;
    ~Item() = default;

    /**
     * @brief Object constructor
     *
     * @param folder - Base folder for DBus object path
     * @param name - DBus object path relative by folder
     * @param args - Default values for each fields
     */
    Item(const std::string& folder, const std::string& name, T&&... args) :
        name(name), data(std::forward<T>(args)...),
        changedMatch(sdbusplus::helper::helper::getBus(),
                     sdbusplus::bus::match::rules::propertiesChanged(
                         folder + "/" + name),
                     std::bind(&Item<T...>::onPropertiesChanged, this,
                               std::placeholders::_1))
    {
    }

    /**
     * @brief PropertiesChanged signal handler
     */
    virtual void onPropertiesChanged(sdbusplus::message::message& m)
    {
        std::string iface;
        fields_map_t data;
        std::vector<std::string> v;
        m.read(iface, data, v);

        setFields(data);
    }

    /**
     * @brief Store fields values recieved from DBus
     */
    virtual void setFields(const fields_map_t& fields) = 0;

    /**
     * @brief Called after object has been created.
     */
    virtual void onCreate()
    {
    }

    /**
     * @brief Called before object will be destroyed.
     */
    virtual void onDestroy()
    {
    }

    /**
     * @brief String fields vlaues helper
     */
    template <size_t Index>
    void setField(const fields_map_t& fieldsMap, const char* propertyName)
    {
        using FieldType = typename std::tuple_element<Index, values_t>::type;
        if (fieldsMap.find(propertyName) != fieldsMap.end() &&
            std::holds_alternative<FieldType>(fieldsMap.at(propertyName)))
        {
            std::get<Index>(data) =
                std::get<FieldType>(fieldsMap.at(propertyName));
        }
    }

    /**
     * @brief Fill snmp reply with fields values
     */
    virtual void get_snmp_reply(netsnmp_agent_request_info* reqinfo,
                                netsnmp_request_info* request) const = 0;

    std::string name;
    values_t data;

  private:
    sdbusplus::bus::match::match changedMatch;
};

/**
 * @brief Used for std::lower_bound throw vector of smartpointers.
 */
template <typename ItemType>
inline bool operator<(const std::unique_ptr<ItemType>& o, const std::string& s)
{
    return o->name < s;
}

} // namespace table
} // namespace data
} // namespace snmp
} // namespace phosphor
