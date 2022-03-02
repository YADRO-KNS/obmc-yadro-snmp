/**
 * @brief SNMP Configuration manager
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
 * @author: Alexander Filippov <a.filippov@yadro.com>
 */

#include <string>
#include <fstream>
#include <streambuf>

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/SNMPCfg/server.hpp>
#include <phosphor-logging/log.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

using namespace phosphor::logging;
using SNMPCfg_inherit = sdbusplus::server::object_t<
    sdbusplus::xyz::openbmc_project::server::SNMPCfg>;

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

static constexpr auto snmpdConf = "/etc/snmp/snmpd.conf";
static constexpr auto whitespace = " \t";

/**
 * @brief Check if the string contains the token at the position or after
 *        several spaces.
 *
 * @param[in] str   - string to be checked
 * @param[in] token - expected token
 * @param[in/out] pos - start position for checking, will keep the last
 *                      checked position.
 *
 * @return - true if token found.
 */
static bool getToken(const std::string& str, const std::string& token,
                     size_t& pos)
{
    pos = str.find_first_not_of(whitespace, pos);
    if (pos != std::string::npos && 0 == str.compare(pos, token.size(), token))
    {
        auto endPos = str.find_first_of(whitespace, pos);
        if (endPos == std::string::npos || pos + token.size() == endPos)
        {
            pos = endPos;
            return true;
        }
    }
    return false;
}

class Configurator : public SNMPCfg_inherit
{
  public:
    /**
     * @brief Configurator object constructor
     *
     * @param bus  - DBus connection object reference
     * @param path - DBus object path
     */
    Configurator(sdbusplus::bus::bus& bus, const char* path) :
        SNMPCfg_inherit(bus, path), bus(bus)
    {
        readConfig();
    }

    std::string community(std::string value) override
    {
        static constexpr auto communityMaxLen = 256;
        static constexpr auto communityMinLen = 1;

        auto communityLen = value.length();
        if (communityLen < communityMinLen || communityLen > communityMaxLen)
        {
            log<level::ERR>("Invalid community name length");
            elog<InvalidArgument>(Argument::ARGUMENT_NAME("Community"),
                                  Argument::ARGUMENT_VALUE(value.c_str()));
        }

        SNMPCfg_inherit::community(value);
        writeConfig();
        return value;
    }

  private:
    /**
     * @brief Parse line and get community name
     *
     * @param line - the line from configuration file
     *
     * @return community name if corresponding statement found
     *         and nullopt otherwise
     */
    std::optional<std::string> getCommunityName(std::string line) const
    {
        // Required line should be in format:
        // 'com2sec readonly  default     <communityName>'
        size_t pos = 0;

        for (const auto& token : {"com2sec", "readonly", "default"})
        {
            if (!getToken(line, token, pos) || pos == std::string::npos)
            {
                return std::nullopt;
            }
        }

        pos = line.find_first_not_of(whitespace, pos);
        if (pos == std::string::npos)
        {
            return std::nullopt;
        }

        auto endPos = line.find_last_not_of(whitespace);
        if (pos < endPos && line[pos] == '"' && line[endPos] == '"')
        {
            pos++;
            endPos--;
        }

        return (pos <= endPos ? line.substr(pos, endPos - pos + 1)
                              : std::string{});
    }

    /**
     * @brief Read actual settings from configuration file
     */
    void readConfig()
    {
        std::ifstream fileToRead(snmpdConf, std::ios::in);
        if (!fileToRead.is_open())
        {
            log<level::ERR>("Failed to open SNMP daemon configuration file",
                            entry("FILE_NAME=%s", snmpdConf));
            return;
        }

        std::string line;
        while (std::getline(fileToRead, line))
        {
            auto communityName = getCommunityName(line);
            if (communityName)
            {
                SNMPCfg_inherit::community(*communityName);
                return;
            }
        }
        log<level::ERR>("Community not found",
                        entry("FILE_NAME=%s", snmpdConf));
    }

    /**
     * @brief Write actual settings to the configuration file.
     */
    void writeConfig()
    {
        std::string tmpFileName{snmpdConf};
        tmpFileName += ".tmp";

        std::ifstream fileToRead(snmpdConf, std::ios::in);
        std::ofstream fileToWrite(tmpFileName, std::ios::out);
        if (!fileToRead.is_open())
        {
            log<level::ERR>("Failed to open SNMP daemon configuration file",
                            entry("FILE_NAME=%s", snmpdConf));
            return;
        }

        if (!fileToWrite.is_open())
        {
            log<level::ERR>("Failed to create new configuration file",
                            entry("FILE_NAME=%s", tmpFileName.c_str()));
            return;
        }

        auto value = SNMPCfg_inherit::community();
        bool isQuoteRequired =
            (value.find_first_of(whitespace) != std::string::npos);
        bool updated = false;
        std::string line;
        while (std::getline(fileToRead, line))
        {
            auto communityName = getCommunityName(line);
            if (communityName && *communityName != value)
            {
                fileToWrite << "com2sec readonly  default\t  "
                            << (isQuoteRequired ? "\"" : "") << value
                            << (isQuoteRequired ? "\"" : "") << std::endl;
                updated = true;
            }
            else
            {
                fileToWrite << line << std::endl;
            }
        }
        fileToWrite.close();
        fileToRead.close();

        if (updated)
        {
            if (0 != std::rename(tmpFileName.c_str(), snmpdConf))
            {
                int error = errno;
                log<level::ERR>("Failed to update SNMP daemon configuarion",
                                entry("WHAT=%s", strerror(error)),
                                entry("FILE_NAME=%s", snmpdConf));
                elog<InternalFailure>();
            }

            reloadSnmpDaemon();
        }
        else
        {
            if (0 != std::remove(tmpFileName.c_str()))
            {
                int error = errno;
                log<level::ERR>("Failed to remove temporary file",
                                entry("WHAT=%s", strerror(error)),
                                entry("FILE_NAME=%s", tmpFileName.c_str()));
            }
        }
    }

    sdbusplus::bus::bus& bus;

    /**
     * @brief Ask SNMP daemon to reread configuration.
     */
    void reloadSnmpDaemon()
    {
        auto m = bus.new_method_call(
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "ReloadUnit");
        m.append("snmpd.service", "replace");
        try
        {
            bus.call_noreply(m);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Failed to reload SNMP daemon",
                            entry("WHATE=%s", e.what()));
            elog<InternalFailure>();
        }
    }
};

/**
 * @brief Application entry point
 *
 * @return exit status
 */
int main()
{
    constexpr auto path = "/xyz/openbmc_project/snmpcfg";

    auto b = sdbusplus::bus::new_default();
    sdbusplus::server::manager_t m{b, path};

    b.request_name("xyz.openbmc_project.SNMPCfg");
    Configurator cfg{b, path};

    while (1)
    {
        b.process_discard();
        b.wait();
    }

    return 0;
}
