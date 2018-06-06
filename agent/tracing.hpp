/**
 * @brief TRACE_* macroses definitions.
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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define TRACE_ERROR(fmt, ...) snmp_log(LOG_ERR, fmt, ##__VA_ARGS__)
#define TRACE_WARNING(fmt, ...) snmp_log(LOG_WARNING, fmt, ##__VA_ARGS__)
#define TRACE_NOTICE(fmt, ...) snmp_log(LOG_NOTICE, fmt, ##__VA_ARGS__)
#define TRACE_INFO(fmt, ...) snmp_log(LOG_INFO, fmt, ##__VA_ARGS__)
#define TRACE_DEBUG(fmt, ...) snmp_log(LOG_DEBUG, fmt, ##__VA_ARGS__)
