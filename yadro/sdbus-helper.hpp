/**
 * @brief sdbusplus helper
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
 * @author: Alexander Filippov <a.filippov@yadro.com>
 */
#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/bus/match.hpp>

#include <tracing.hpp>
#include <sdbus-remote.hpp>

namespace sdbusplus
{
namespace helper
{

constexpr auto OBJECT_MAPPER_IFACE = "xyz.openbmc_project.ObjectMapper";
constexpr auto OBJECT_MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto PROPETIES_IFACE = "org.freedesktop.DBus.Properties";

class helper
{
  public:
    helper(const char* host = nullptr) :
        m_bus(sdbusplus::bus::open_system(host)), m_running(true)
    {
    }

    /** @brief Invoke a method. */
    template <typename... Args>
    auto callMethod(const std::string& busName, const std::string& path,
                    const std::string& interface, const std::string& method,
                    Args&&... args)
    {
        auto reqMsg = m_bus.new_method_call(busName.c_str(), path.c_str(),
                                            interface.c_str(), method.c_str());
        reqMsg.append(std::forward<Args>(args)...);
        auto respMsg = m_bus.call(reqMsg);

        if (respMsg.is_method_error())
        {
            TRACE_ERROR("Failed to invoke DBus method. "
                        "PATH=%s, INTERFACE=%s, METHOD=%s\n",
                        path.c_str(), interface.c_str(), method.c_str());
        }

        return respMsg;
    }

    /** @brief Invoke a method and read the response. */
    template <typename Ret, typename... Args>
    auto callMethodAndRead(const std::string& busName, const std::string& path,
                           const std::string& interface,
                           const std::string& method, Args&&... args)
    {
        sdbusplus::message::message respMsg = callMethod<Args...>(
            busName, path, interface, method, std::forward<Args>(args)...);
        Ret resp;
        respMsg.read(resp);
        return resp;
    }

    /** @brief Get subtree from the mapper. */
    auto getSubTree(const std::string& path, const std::string& interface,
                    int32_t depth)
    {
        using namespace std::literals::string_literals;

        using Path = std::string;
        using Intf = std::string;
        using Serv = std::string;
        using Intfs = std::vector<Intf>;
        using Objects = std::map<Path, std::map<Serv, Intfs>>;
        Intfs intfs;
        if (!interface.empty())
        {
            intfs.push_back(interface);
        }

        auto mapperResp = callMethodAndRead<Objects>(
            OBJECT_MAPPER_IFACE, OBJECT_MAPPER_PATH, OBJECT_MAPPER_IFACE,
            "GetSubTree", path, depth, intfs);

        if (mapperResp.empty())
        {
            TRACE_ERROR("Empty response from mapper GetSubTree: "
                        "SUBTREE=%s, INTERFACE=%s, DEPTH=%d\n",
                        path.c_str(), interface.c_str(), depth);
        }

        return mapperResp;
    }

    /** @brief Get a property. */
    template <typename Property>
    auto getProperty(const std::string& busName, const std::string& path,
                     const std::string& interface, const std::string& property)
    {
        auto reqMsg = callMethod(busName, path, PROPETIES_IFACE, "Get",
                                 interface, property);
        sdbusplus::message::variant<Property> value;
        reqMsg.read(value);
        return value.template get<Property>();
    }

    /** @brief Check if running flag is set. */
    bool isRunning(void) const
    {
        return m_running;
    }

    /** @brief Terminate loop. */
    void terminate(void)
    {
        m_running = false;
    }

    /** @brief Start loop and handle deferred DBus calls/signals. */
    void loop(void)
    {
        while (isRunning())
        {
            m_bus.process_discard();
            if (isRunning())
            {
                m_bus.wait(100000 /*microseconds*/);
            }
        }
    }

  protected:
    // Member variables

    sdbusplus::bus::bus m_bus; //!< DBus connection
    volatile bool m_running;   //!< Running flag
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
           interface(sdbusplus::helper::PROPETIES_IFACE);
}
} // namespace rules
} // namespace match
} // namespace bus
} // namespace sdbusplus
