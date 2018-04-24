/**
 * @brief SNMP Tables definitions
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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* column number definitions for sensors table */
#define COLUMN_YADROSENSOR_NAME 1
#define COLUMN_YADROSENSOR_VALUE 2
#define COLUMN_YADROSENSOR_WARNLOW 3
#define COLUMN_YADROSENSOR_WARNHIGH 4
#define COLUMN_YADROSENSOR_CRITLOW 5
#define COLUMN_YADROSENSOR_CRITHIGH 6
#define COLUMN_YADROSENSOR_STATE 7

void init_yadroTables(void);
void drop_yadroTables(void);
