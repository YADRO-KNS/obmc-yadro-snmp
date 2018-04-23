/**
 * @brief SNMP Configuration manager
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

#include <string>
#include <fstream>
#include <streambuf>

#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/SNMPCfg/server.hpp>

using SNMPCfg_inherit = sdbusplus::server::object_t<sdbusplus::xyz::openbmc_project::server::SNMPCfg>;

struct Configurator : SNMPCfg_inherit
{
    const std::string snmpd_conf_path = "/etc/snmp/snmpd.conf";
    const std::string snmpd_orig_path = "/run/initramfs/ro/etc/snmp/snmpd.conf";

    const std::string systemd_dst   = "org.freedesktop.systemd1";
    const std::string systemd_path  = "/org/freedesktop/systemd1";
    const std::string systemd_iface = "org.freedesktop.systemd1.Manager";

    /**
     * @brief Configurator object constructor
     *
     * @param bus  - DBus connection object reference
     * @param path - DBus object path
     */
    Configurator(sdbusplus::bus::bus& bus, const char* path)
        : SNMPCfg_inherit(bus, path)
    {
    }

    /**
     * @brief Get actual version of snmpd.conf
     *
     * @return snmpd.conf body
     */
    virtual std::string getConfig(void) override
    {
        std::ifstream t(snmpd_conf_path);
        std::string s;

        t.seekg(0, std::ios::end);
        s.reserve(t.tellg());
        t.seekg(0, std::ios::beg);

        s.assign((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());

        return s;
    }

    /**
     * @brief Set new version of snmpd.conf
     *
     * @param conf - body of snmpd.conf
     */
    virtual void setConfig(std::string conf) override
    {
        if (dbus_call("StopUnit"))
        {
            std::ofstream t(snmpd_conf_path);
            t << conf;
            t.close();

            dbus_call("StartUnit");
        }
    }

    /**
     * @brief Rollback snmpd.conf to default version
     */
    virtual void resetConfig(void) override
    {
        if (dbus_call("StopUnit"))
        {
            std::ifstream src(snmpd_orig_path);
            std::ofstream dst(snmpd_conf_path);

            dst << src.rdbuf();

            dst.close();
            src.close();

            dbus_call("StartUnit");
        }
    }

private:

    /**
     * @brief Send systemd command over DBus
     *
     * @param method - Method name (StartUnit | StopUnit)
     *
     * @return true if success
     */
    bool dbus_call (const char * method)
    {
        auto b = sdbusplus::bus::new_system();
        auto m = b.new_method_call("org.freedesktop.systemd1",
                                   "/org/freedesktop/systemd1",
                                   "org.freedesktop.systemd1.Manager",
                                   method);
        m.append("snmpd.service", "replace");
        auto r = b.call(m);
        if (r.is_method_error())
        {
            fprintf(stderr, "Error: %s() failed!\n", method);
            return false;
        }

#ifdef _DEBUG
        sdbusplus::message::object_path o;
        r.read(o);
        printf("  %s() return '%s'\n",
                method, o.str.c_str());
#endif

        return true;
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
