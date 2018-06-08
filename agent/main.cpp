/**
 * @brief SNMP Agent entry point
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

#include "config.h"
#include "tracing.hpp"
#include "sdevent/event.hpp"
#include "sdbusplus/helper.hpp"
#include "snmp.hpp"

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <csignal>

#include "yadro/powerstate.hpp"
#include "yadro/sensors.hpp"
#include "yadro/software.hpp"

void print_usage()
{
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", PACKAGE_NAME);
    fprintf(stderr, "  Version:  %s\n\nOPTIONS:\n", PACKAGE_VERSION);
    fprintf(stderr, "  -h,--help\t\tdisplay this help message\n");
    fprintf(stderr, "  -d\t\t\tdump sent and received SNMP packets\n");
    fprintf(
        stderr,
        "  -D[TOKEN[,...]]\tturn on debugging output for the given TOKEN(s)\n"
        "\t\t\t   (try ALL for extremely verbose output)\n");
    fprintf(stderr,
            "  -L <LOGOPTS>\t\ttoggle options controlling where to log to\n");
    snmp_log_options_usage("\t\t\t  ", stderr);
    fflush(stderr);
}

// parse_args error codes
#define EC_SHOW_USAGE    1
#define EC_ERROR        -1
#define EC_SUCCESS       0

int parse_args(int argc, char** argv)
{
    constexpr auto Opts = "dD:L:h";

    optind = 1;
    int arg;
    int rc = EC_SUCCESS;

    while (EC_SUCCESS == rc && (arg = getopt(argc, argv, Opts)) != EOF)
    {
        DEBUGMSGTL(("parse_args", "handling (#%d): %c\n", optind, arg));
        switch (arg)
        {
            case 'D':
                debug_register_tokens(optarg);
                snmp_set_do_debugging(1);
                break;

            case 'd':
                netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID,
                                       NETSNMP_DS_LIB_DUMP_PACKET, 1);
                break;

            case 'L':
                rc = snmp_log_options(optarg, argc, argv);
                break;

            case 'h':
                rc = EC_SHOW_USAGE;
                break;

            case '?':
            default:
                rc = EC_ERROR;
                break;
        }
    }
    DEBUGMSGTL(("parse_args", "finished: %d/%d\n", optind, argc));
    return rc;
}

void setup_signals(sdevent::event::Event& evt)
{
    sigset_t ss;
    if (sigemptyset(&ss) < 0 || sigaddset(&ss, SIGTERM) < 0 ||
        sigaddset(&ss, SIGINT) < 0)
    {
        TRACE_ERROR("Failed to setup signals hanlders.\n");
        return;
    }

    /* Block SIGTERM first, so than the event loop can handle it */
    if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
    {
        TRACE_ERROR("Failed to block SIGTERM.\n");
        return;
    }

    evt.add_signal(SIGTERM, nullptr);
    evt.add_signal(SIGINT, nullptr);
}

int main(int argc, char* argv[])
{
    int rc = parse_args(argc, argv);
    if (rc < EC_SUCCESS)
    {
        exit(EXIT_FAILURE);
    }
    else if (rc > EC_SUCCESS)
    {
        print_usage();
        exit(EXIT_SUCCESS);
    }

    auto& evt = sdevent::event::get_default();
    evt.attach(sdbusplus::helper::helper::getBus());

    TRACE_INFO("%s is up and running.\n", PACKAGE_STRING);
    snmpagent_init();

    // Initialize DBus and MIB objects

    yadro::host::power::state::init();
    yadro::sensors::init();
    yadro::software::init();

    setup_signals(evt);

    // main loop
    while (rc >= 0)
    {
        snmpagent_run();
        rc = evt.run(WAIT_INFINITE);
    }

    TRACE_INFO("%s shuting down.\n", PACKAGE_STRING);

    // Release DBus and MIB objects resources

    yadro::software::destroy();
    yadro::sensors::destroy();
    yadro::host::power::state::destroy();

    snmpagent_destroy();

    return rc;
}
