/**
 * @brief Export DBus objects tree to MIB table
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
#pragma once

#include "sdbusplus/helper.hpp"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

namespace phosphor
{
namespace snmp
{
namespace data
{

/**
 * @brief MIB Table implementation.
 */
template <typename ItemType> class Table
{
  public:
    using interfaces_t = std::vector<std::string>;

    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    Table() = delete;
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = default;
    Table& operator=(Table&&) = default;
    ~Table() = default;

    /**
     * @brief Object constructor
     *
     * @param folder - DBus folder wherefrom signals about add/remove objects
     * will be recieved.
     * @param subfolder - Using only objects from subfolder
     * @param interfaces - List of required DBus properties interfaces
     */
    Table(const std::string& folder, const std::string& subfolder,
          const interfaces_t interfaces = {}) :
        _path(folder + (subfolder.empty() ? "" : "/" + subfolder)),
        _interfaces(interfaces)
    {
        _matches.emplace_back(
            sdbusplus::helper::helper::getBus(),
            sdbusplus::bus::match::rules::interfacesAdded(folder),
            std::bind(&Table<ItemType>::onInterfacesAdded, this,
                      std::placeholders::_1));
        _matches.emplace_back(
            sdbusplus::helper::helper::getBus(),
            sdbusplus::bus::match::rules::interfacesRemoved(folder),
            std::bind(&Table<ItemType>::onInterfacesRemoved, this,
                      std::placeholders::_1));
    }

    /**
     * @brief Force update table items
     */
    void update()
    {
        auto data = sdbusplus::helper::helper::getSubTree(_path, _interfaces);

        // Drop sensors if it not present in answer
        for (auto it = _items.begin(); it != _items.end();)
        {
            if (data.find((*it)->path) == data.end())
            {
                it = dropItem(it);
            }
            else
            {
                ++it;
            }
        }

        // Update existing and create new items.
        using fields_map_t = typename ItemType::fields_map_t;
        for (const auto& pi : data)
        {
            for (const auto& bi : pi.second)
            {
                auto fields =
                    sdbusplus::helper::helper::callMethodAndRead<fields_map_t>(
                        bi.first, pi.first, sdbusplus::helper::PROPERTIES_IFACE,
                        "GetAll", "");
                getItem(pi.first).setFields(fields);
            }
        }
    }

    /**
     * @brief Register MIB handlers
     *
     * @param name - Name of table in MIB
     * @param table_oid - OID of table
     * @param table_oid_len - Table OID length
     * @param min_column - Minimum columns number
     * @param max_column - Maximum columns number
     */
    void init_mib(const char* name, const oid* table_oid, size_t table_oid_len,
                  size_t min_column, size_t max_column)
    {
        netsnmp_handler_registration* reg = netsnmp_create_handler_registration(
            name, Table<ItemType>::snmp_handler, table_oid, table_oid_len,
            HANDLER_CAN_RONLY);

        netsnmp_table_registration_info* table_info =
            SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
        netsnmp_table_helper_add_indexes(table_info, ASN_OCTET_STR, 0);
        table_info->min_column = min_column;
        table_info->max_column = max_column;

        netsnmp_iterator_info* iinfo =
            SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
        iinfo->get_first_data_point = Table<ItemType>::get_first_data_point;
        iinfo->get_next_data_point = Table<ItemType>::get_next_data_point;
        iinfo->table_reginfo = table_info;
        iinfo->myvoid = this;

        netsnmp_register_table_iterator(reg, iinfo);
    }

  protected:
    using ItemPtr = std::unique_ptr<ItemType>;
    using Items = std::vector<ItemPtr>;

    /**
     * @brief DBus signal `InterfacesAdded` handler.
     */
    void onInterfacesAdded(sdbusplus::message::message& m)
    {
        sdbusplus::message::object_path path;
        std::map<std::string, typename ItemType::fields_map_t> data;
        m.read(path, data);

        if (0 == path.str.compare(0, _path.length(), _path))
        {
            auto& item = getItem(path.str);

            for (auto it = data.cbegin(); it != data.cend(); ++it)
            {
                item.setFields(it->second);
            }
        }
    }

    /**
     * @brief DBus signal `InterfacesRemoved` handler.
     */
    void onInterfacesRemoved(sdbusplus::message::message& m)
    {
        sdbusplus::message::object_path path;
        std::vector<std::string> data;
        try
        {
            m.read(path, data);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            TRACE_ERROR("data/table: Failed to parse signal data. "
                        "ERROR='%s', REPLY_SIG='%s', PATH='%s', "
                        "IFACE='%s', MEMBER='%s', object path='%s'\n",
                        e.what(), m.get_signature(), m.get_path(),
                        m.get_interface(), m.get_member(), path.str.c_str());
        }
        dropItem(path.str);
    }

    /**
     * @brief Create new item if does not exist and return reference.
     */
    ItemType& getItem(const std::string& path)
    {
        auto it = std::lower_bound(_items.begin(), _items.end(), path);
        if (it != _items.end() && (*it)->path == path)
        {
            return *(*it);
        }

        it = _items.emplace(it, std::make_unique<ItemType>(path));
        (*it)->onCreate();
        return *(*it);
    }

    /**
     * @brief Drop item specified by iterator.
     *
     * @param it - Items iterator
     *
     * @return Iterator pointing to the new location of the item that followed
     * the item erased.
     */
    typename Items::iterator dropItem(typename Items::iterator it)
    {
        if (it != _items.end())
        {
            (*it)->onDestroy();
            it = _items.erase(it);
        }
        return it;
    }

    /**
     * @brief Drop item.
     */
    void dropItem(const std::string& path)
    {
        auto it = std::lower_bound(_items.begin(), _items.end(), path);
        if (it != _items.end() && (*it)->path == path)
        {
            dropItem(it);
        }
    }

    /**
     * @brief Iterator hook routines
     */
    static netsnmp_variable_list*
        get_first_data_point(void** loop_ctx, void** data_ctx,
                             netsnmp_variable_list* idx_data,
                             netsnmp_iterator_info* data)
    {
        *loop_ctx = reinterpret_cast<void*>(0);
        return Table<ItemType>::get_next_data_point(loop_ctx, data_ctx,
                                                    idx_data, data);
    }
    static netsnmp_variable_list*
        get_next_data_point(void** loop_ctx, void** data_ctx,
                            netsnmp_variable_list* idx_data,
                            netsnmp_iterator_info* data)
    {
        auto index = reinterpret_cast<size_t>(*loop_ctx);
        auto& table = *reinterpret_cast<Table<ItemType>*>(data->myvoid);

        if (index < table._items.size())
        {
            auto& item = table._items[index];

            auto name = item->path.substr(table._path.length() + 1);
            snmp_set_var_value(idx_data, name.c_str(), name.length());

            *data_ctx = item.get();
            *loop_ctx = reinterpret_cast<void*>(index + 1);
            return idx_data;
        }

        return nullptr;
    }

    /**
     * @brief handles snmp requests for the table data.
     */
    static int snmp_handler(netsnmp_mib_handler* handler,
                            netsnmp_handler_registration* reginfo,
                            netsnmp_agent_request_info* reqinfo,
                            netsnmp_request_info* requests)
    {
        switch (reqinfo->mode)
        {
            case MODE_GET:
                for (auto request = requests; request; request = request->next)
                {
                    auto entry = reinterpret_cast<ItemType*>(
                        netsnmp_extract_iterator_context(request));

                    if (!entry)
                    {
                        netsnmp_set_request_error(reqinfo, request,
                                                  SNMP_NOSUCHINSTANCE);
                        continue;
                    }

                    entry->get_snmp_reply(reqinfo, request);
                }
                break;
        }

        return SNMP_ERR_NOERROR;
    }

    std::string _path;
    interfaces_t _interfaces;
    std::vector<sdbusplus::bus::match::match> _matches;
    Items _items;
};

} // namespace data
} // namespace snmp
} // namespace phosphor
