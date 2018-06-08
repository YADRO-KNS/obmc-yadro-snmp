/**
 * @brief DBus enums to base type converter.
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

#include <string>
#include <map>

namespace phosphor
{
namespace snmp
{
namespace data
{

template <typename T> struct DBusEnum
{
    std::string base;
    std::map<std::string, T> values;
    T wrongValue;

    T get(const std::string& str) const
    {
        const auto len = base.length();
        if (0 == str.compare(0, len, base))
        {
            const auto& it = values.find(str.substr(len + 1));
            if (it != values.end())
            {
                return it->second;
            }
        }

        return wrongValue;
    }
};

} // namespace data
} // namespace snmp
} // namespace phosphor
