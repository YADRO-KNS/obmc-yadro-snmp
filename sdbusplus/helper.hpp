/**
 * @brief sdbusplus library helper
 *
 * This file is part of yadro-snmp-agent project.
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

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace helper
{

constexpr auto OBJECT_MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto OBJECT_MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto PROPERTIES_IFACE = "org.freedesktop.DBus.Properties";

struct helper
{
    static auto& getBus()
    {
        static auto bus = sdbusplus::bus::new_system();
        return bus;
    }

    /** @brief Invoke a method. */
    template <typename... Args>
    static auto callMethod(const std::string& busName, const std::string& path,
                           const std::string& interface,
                           const std::string& method, Args&&... args)
    {
        auto reqMsg = getBus().new_method_call(
            busName.c_str(), path.c_str(), interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        return getBus().call(reqMsg);
    }

    /** @brief Invoke a method and read the response. */
    template <typename Ret, typename... Args>
    static Ret callMethodAndRead(const std::string& busName,
                                 const std::string& path,
                                 const std::string& interface,
                                 const std::string& method, Args&&... args)
    {
        Ret resp;
        try
        {
            sdbusplus::message::message respMsg = callMethod<Args...>(
                busName, path, interface, method, std::forward<Args>(args)...);
            respMsg.read(resp);
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
        }
        return resp;
    }

    using Path = std::string;
    using Interface = std::string;
    using Service = std::string;
    using Interfaces = std::vector<Interface>;
    using Services = std::map<Service, Interfaces>;
    using Objects = std::map<Path, Services>;

    /** @brief Get subtree from the mapper. */
    static Objects getSubTree(const std::string& path, const Interfaces& ifaces,
                              int32_t depth = 0)
    {
        using namespace std::literals::string_literals;

        return callMethodAndRead<Objects>(
            OBJECT_MAPPER_IFACE, OBJECT_MAPPER_PATH, OBJECT_MAPPER_IFACE,
            "GetSubTree", path, depth, ifaces);
    }

    /** @brief Get subtree paths from mapper. */
    static Interfaces getSubTreePaths(const std::string& path,
                                      const Interfaces& ifaces,
                                      int32_t depth = 0)
    {
        return callMethodAndRead<Interfaces>(
            OBJECT_MAPPER_IFACE, OBJECT_MAPPER_PATH, OBJECT_MAPPER_IFACE,
            "GetSubTreePaths", path, depth, ifaces);
    }

    /** @brief Get service provides specified object */
    static Service getService(const Path& path, const Interface& iface)
    {
        Interfaces ifaces = {iface};
        auto services = callMethodAndRead<Services>(
            OBJECT_MAPPER_IFACE, OBJECT_MAPPER_PATH, OBJECT_MAPPER_IFACE,
            "GetObject", path, ifaces);

        if (!services.empty())
        {
            return std::move(services.begin()->first);
        }

        return {};
    }

    /** @brief Get a property. */
    template <typename Property>
    static Property
        getProperty(const std::string& busName, const std::string& path,
                    const std::string& interface, const std::string& property)
    {
        auto reqMsg = callMethod(busName, path, PROPERTIES_IFACE, "Get",
                                 interface, property);
        std::variant<Property> value;
        reqMsg.read(value);
        return value.template get<Property>();
    }

    /** @brief Get all properties. */
    template <typename... Types>
    static auto getAllProperties(const std::string& busName,
                                 const std::string& path,
                                 const std::string& interface)
    {
        using Value = std::variant<Types...>;
        using Values = std::map<std::string, Value>;

        auto reqMsg =
            callMethod(busName, path, PROPERTIES_IFACE, "GetAll", interface);

        Values values;
        reqMsg.read(values);
        return values;
    }
};

} // namespace helper

namespace bus
{
namespace match
{
namespace rules
{
inline auto propertiesChanged(const std::string& p)
{
    return type::signal() + path(p) + member("PropertiesChanged") +
           interface(sdbusplus::helper::PROPERTIES_IFACE);
}
} // namespace rules
} // namespace match
} // namespace bus
} // namespace sdbusplus
