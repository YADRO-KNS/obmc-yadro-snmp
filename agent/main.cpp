/**
 * @brief SNMP Agent entry point
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
#include "sdbusplus/helper.hpp"
#include "snmp.hpp"

#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <csignal>

#include "yadro/powerstate.hpp"
#include "yadro/sensors.hpp"
#include "yadro/software.hpp"
#include "yadro/inventory.hpp"

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
enum
{
    EC_SHOW_USAGE = 1,
    EC_ERROR = -1,
    EC_SUCCESS = 0,
};

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

static void clean_exit(sdeventplus::source::Signal& source,
                       const struct signalfd_siginfo*)
{
    TRACE_INFO("Signal %d received, terminating...\n", source.get_signal());
    source.get_event().exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    int rc = parse_args(argc, argv);
    if (rc < EC_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    else if (rc > EC_SUCCESS)
    {
        print_usage();
        return EXIT_SUCCESS;
    }

    auto evt = sdeventplus::Event::get_default();
    sdbusplus::helper::helper::getBus().attach_event(evt.get(),
                                                     SD_EVENT_PRIORITY_NORMAL);

    sigset_t ss;
    if (sigemptyset(&ss) < 0 || sigaddset(&ss, SIGTERM) < 0 ||
        sigaddset(&ss, SIGINT) < 0)
    {
        TRACE_ERROR("Failed to setup signal hanlders.\n");
        return EXIT_FAILURE;
    }

    /* Block SIGTERM first, so than the event loop can handle it */
    if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
    {
        TRACE_ERROR("Failed to block SIGTERM.\n");
        return EXIT_FAILURE;
    }

    sdeventplus::source::Signal sigterm(evt, SIGTERM, clean_exit);
    sdeventplus::source::Signal sigint(evt, SIGINT, clean_exit);

    snmpagent_init(evt);

    // Initialize DBus and MIB objects

    yadro::host::power::state::init();
    yadro::sensors::init();
    yadro::software::init();
    yadro::inventory::init();

    // main loop

    TRACE_INFO("%s is up and running.\n", PACKAGE_STRING);

    rc = evt.loop();

    TRACE_INFO("%s shuting down.\n", PACKAGE_STRING);

    // Release DBus and MIB objects resources

    yadro::inventory::destroy();
    yadro::software::destroy();
    yadro::sensors::destroy();
    yadro::host::power::state::destroy();

    snmpagent_destroy();

    if (rc < 0)
    {
        TRACE_ERROR("Event loop returned error %d, %s\n", rc, strerror(-rc));
        return -rc;
    }
    return EXIT_SUCCESS;
}
