/**
 * @brief Remote host DBus connector
 *
 * This file is part of phosphor-snmp project.
 *
 * Copyright (c) 2018 YADRO (KNS Group LLC)
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

namespace sdbusplus
{
namespace bus
{
/**
 * @brief if specified host, using ssh for connec to dbus on specified host
 *
 * @param host - hostname optionaly with user name
 *
 * @return dbus connection object
 */
inline bus open_system(const char* host = nullptr)
{
    if (!host)
    {
        return new_system();
    }

    TRACE("Open DBus session to %s\n", host);
    sd_bus* b = nullptr;
    sd_bus_open_system_remote(&b, host);
    return bus(b, std::false_type());
}

} // namespace bus
} // namespace sdbusplus

