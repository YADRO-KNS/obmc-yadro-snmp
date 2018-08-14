/**
 * @brief SNMP Agent implementation.
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
#include "config.h"
#include "tracing.hpp"
#include "sdevent/event.hpp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <map>

// list of snmp file descriptors attached to sd_event loop.
static std::map<int, sdevent::source::Source> snmp_fds;

/** @brief Called when snmp file descriptors have data for reading. */
static int sdevent_snmp_read(sd_event_source* /*es*/, int fd,
                             uint32_t /*revents*/, void* /*userdata*/)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    snmp_read(&fdset);

    return 0;
}

/** @brief Refresh list of snmp file descriptors. */
static void sdevent_snmp_update()
{
    int maxfd = 0;
    int is_blocked = 1;
    fd_set fdset;
    timeval timeout;

    FD_ZERO(&fdset);
    snmp_select_info(&maxfd, &fdset, &timeout, &is_blocked);

    // We need to untrack any event whose FD is not in `fdset` anymore.
    for (auto it = snmp_fds.begin(); it != snmp_fds.end();)
    {
        if (it->first >= maxfd || (!FD_ISSET(it->first, &fdset)))
        {
            it->second.enable(0);
            it = snmp_fds.erase(it);
        }
        else
        {
            FD_CLR(it->first, &fdset);
            ++it;
        }
    }

    auto& event = sdevent::event::get_default();

    // Invariant: FD in `fdset` are not in `snmp_fds`
    for (int fd = 0; fd < maxfd; ++fd)
    {
        if (FD_ISSET(fd, &fdset))
        {
            snmp_fds.emplace(
                fd, event.add_io(fd, EPOLLIN, sdevent_snmp_read, nullptr));
        }
    }
}

/** @brief Initialize snmp agent */
void snmpagent_init()
{
    // make us a agentx client
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

    // initialize tcpip, if necessary
    SOCK_STARTUP;

    // initialize the agent library
    init_agent(PACKAGE_NAME);

    // We will be used to read <PACKAGE_NAME>.conf files.
    init_snmp(PACKAGE_NAME);
}

/** @brief snmpagent main loop iteration. */
void snmpagent_run()
{
    netsnmp_check_outstanding_agent_requests();
    sdevent_snmp_update();
}

/** @brief Deinitialize snmp agen */
void snmpagent_destroy()
{
    snmp_shutdown(PACKAGE_NAME);
    SOCK_CLEANUP;
}
