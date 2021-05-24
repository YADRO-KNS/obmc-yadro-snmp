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

#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/source/time.hpp>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <map>

constexpr auto clockId = sdeventplus::ClockId::Monotonic;
using Clock = sdeventplus::Clock<clockId>;
using Time = sdeventplus::source::Time<clockId>;

// list of snmp file descriptors attached to sd_event loop.
static std::map<int, sdeventplus::source::IO> snmp_fds;

/** @brief Called when snmp file descriptors have data for reading. */
static void sdevent_snmp_read(sdeventplus::source::IO& /*source*/, int fd,
                              uint32_t /*revents*/)
{
    DEBUGMSGTL(("snmpagent::handle", "Handle fd=%d\n", fd));
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    snmp_read(&fdset);
}

/** @brief Refresh list of snmp file descriptors. */
static void sdevent_snmp_update(const sdeventplus::Event& event)
{
    netsnmp_check_outstanding_agent_requests();

    int maxfd = 0;
    int is_blocked = 1;
    fd_set fdset;
    timeval timeout;

    FD_ZERO(&fdset);
    snmp_select_info(&maxfd, &fdset, &timeout, &is_blocked);

    static Time snmpTimer(event, {}, std::chrono::microseconds{1},
                          [](Time&, Time::TimePoint) {
                              DEBUGMSGTL(("snmpagent::handle", "Time out"));
                              snmp_timeout();
                              run_alarms();
                          });

    if (timeout.tv_sec || timeout.tv_usec)
    {
        snmpTimer.set_time(Clock(event).now() +
                           std::chrono::seconds{timeout.tv_sec} +
                           std::chrono::microseconds{timeout.tv_usec});
        snmpTimer.set_enabled(sdeventplus::source::Enabled::OneShot);
    }

    // We need to untrack any event whose FD is not in `fdset` anymore.
    for (auto it = snmp_fds.begin(); it != snmp_fds.end();)
    {
        if (it->first >= maxfd || (!FD_ISSET(it->first, &fdset)))
        {
            DEBUGMSGTL(
                ("snmpagent::handle", "Remove fd=%d from set.\n", it->first));
            it->second.set_enabled(sdeventplus::source::Enabled::Off);
            it = snmp_fds.erase(it);
        }
        else
        {
            FD_CLR(it->first, &fdset);
            ++it;
        }
    }

    // Invariant: FD in `fdset` are not in `snmp_fds`
    for (int fd = 0; fd < maxfd; ++fd)
    {
        if (FD_ISSET(fd, &fdset))
        {
            DEBUGMSGTL(("snmpagent::handle", "Add fd=%d to set.\n", fd));
            snmp_fds.emplace(fd, sdeventplus::source::IO(event, fd, EPOLLIN,
                                                         sdevent_snmp_read));
        }
    }
}

/** @brief Initialize snmp agent */
void snmpagent_init(const sdeventplus::Event& event)
{
    // make us a agentx client
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

    // initialize tcpip, if necessary
    SOCK_STARTUP;

    // initialize the agent library
    init_agent(PACKAGE_NAME);

    // We will be used to read <PACKAGE_NAME>.conf files.
    init_snmp(PACKAGE_NAME);

    // This is run before the event loop will sleep and wait for new events.
    static sdeventplus::source::Post post(
        event, [](sdeventplus::source::EventBase& source) {
            sdevent_snmp_update(source.get_event());
        });

    sdevent_snmp_update(event);
}

/** @brief Deinitialize snmp agen */
void snmpagent_destroy()
{
    snmp_shutdown(PACKAGE_NAME);
    SOCK_CLEANUP;
}
