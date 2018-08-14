# yadro-snmp

This project containt two subprojects:
- [yadro-snmp-agent](#yadro-snmp-agent)
- [snmpcfg - DBus service for manage snmp configuration](#snmpcfg)

## yadro-snmp-agent

This is a subagent for the original net-snmp daemon. 
It queries DBus for values of available sensors, inventory items and some 
other stuff, and exports their actual values over SNMP.

### YADRO-MIB.txt

File YADOR-MIB.txt describe a struc of exported data.
This file can be found at OpenBMC hosts in folder `/usr/share/snmp/mibs` and may be fetched from OpenBMC host over http.
```shell
$ wget https://<IP-addres-of-BMC-host>/mibs/YADRO-MIB.txt
```

### Basic SNMP 

This module export a host power state field and tree tables of sensors values.
```shell
$ snmptranslate -Tp -IR YADRO-MIB::yadro
+--yadro(49769)
   |
   +--yadroNotifications(0)
   |  |
   |  +--yadroHostPowerStateNotification(1)
   |  +--yadroTempSensorStateNotification(2)
   |  +--yadroVoltSensorStateNotification(3)
   |  +--yadroTachSensorStateNotification(4)
   |
   +--yadroSensors(1)
   |  |
   |  +-- -R-- EnumVal   yadroHostPowerState(1)
   |  |        Values: unknown(-1), off(0), on(1)
   |  |
   |  +--yadroTempSensorsTable(2)
   |  |  |
   |  |  +--yadroTempSensorsEntry(1)
   |  |     |  Index: yadroTempSensorName
   |  |     |
   |  |     +-- ---- String    yadroTempSensorName(1)
   |  |     |        Size: 1..32
   |  |     +-- -R-- Integer32 yadroTempSensorValue(2)
   |  |     |        Textual Convention: Degrees
   |  |     +-- -R-- Integer32 yadroTempSensorWarnLow(3)
   |  |     |        Textual Convention: Degrees
   |  |     +-- -R-- Integer32 yadroTempSensorWarnHigh(4)
   |  |     |        Textual Convention: Degrees
   |  |     +-- -R-- Integer32 yadroTempSensorCritLow(5)
   |  |     |        Textual Convention: Degrees
   |  |     +-- -R-- Integer32 yadroTempSensorCritHigh(6)
   |  |     |        Textual Convention: Degrees
   |  |     +-- -R-- EnumVal   yadroTempSensorState(7)
   |  |              Textual Convention: SensorState
   |  |              Values: disabled(0), normal(1), warningLow(2), warningHigh(3), criticalLow(4), criticalHigh(5)
   |  |
   |  +--yadroVoltSensorsTable(3)
   |  |  |
   |  |  +--yadroVoltSensorsEntry(1)
   |  |     |  Index: yadroVoltSensorName
   |  |     |
   |  |     +-- ---- String    yadroVoltSensorName(1)
   |  |     |        Size: 1..32
   |  |     +-- -R-- Integer32 yadroVoltSensorValue(2)
   |  |     |        Textual Convention: Voltage
   |  |     +-- -R-- Integer32 yadroVoltSensorWarnLow(3)
   |  |     |        Textual Convention: Voltage
   |  |     +-- -R-- Integer32 yadroVoltSensorWarnHigh(4)
   |  |     |        Textual Convention: Voltage
   |  |     +-- -R-- Integer32 yadroVoltSensorCritLow(5)
   |  |     |        Textual Convention: Voltage
   |  |     +-- -R-- Integer32 yadroVoltSensorCritHigh(6)
   |  |     |        Textual Convention: Voltage
   |  |     +-- -R-- EnumVal   yadroVoltSensorState(7)
   |  |              Textual Convention: SensorState
   |  |              Values: disabled(0), normal(1), warningLow(2), warningHigh(3), criticalLow(4), criticalHigh(5)
   |  |
   |  +--yadroTachSensorsTable(4)
   |     |
   |     +--yadroTachSensorsEntry(1)
   |        |  Index: yadroTachSensorName
   |        |
   |        +-- ---- String    yadroTachSensorName(1)
   |        |        Size: 1..32
   |        +-- -R-- Integer32 yadroTachSensorValue(2)
   |        |        Textual Convention: RPMS
   |        +-- -R-- Integer32 yadroTachSensorWarnLow(3)
   |        |        Textual Convention: RPMS
   |        +-- -R-- Integer32 yadroTachSensorWarnHigh(4)
   |        |        Textual Convention: RPMS
   |        +-- -R-- Integer32 yadroTachSensorCritLow(5)
   |        |        Textual Convention: RPMS
   |        +-- -R-- Integer32 yadroTachSensorCritHigh(6)
   |        |        Textual Convention: RPMS
   |        +-- -R-- EnumVal   yadroTachSensorState(7)
   |                 Textual Convention: SensorState
   |                 Values: disabled(0), normal(1), warningLow(2), warningHigh(3), criticalLow(4), criticalHigh(5)
   |
   +--yadroConformance(2)
      |
      +--yadroCompliances(1)
      |  |
      |  +--yadroCommpliance(1)
      |
      +--yadroGroups(2)
         |
         +--yadroScalarFieldsGroup(1)
         +--yadroTempSensorsTableGroup(2)
         +--yadroVoltSensorsTableGroup(3)
         +--yadroTachSensorsTableGroup(4)
         +--yadroNotificationsGroup(10)
``` 

Content of tables may be viewed by command `snmptable`:
```shell
$ snmptable -v2c -cpublic -Ci manzoni.dev.yadro.com YADRO-MIB::yadroTempSensorsTable
SNMP table: YADRO-MIB::yadroTempSensorsTable

         index yadroTempSensorValue yadroTempSensorWarnLow yadroTempSensorWarnHigh yadroTempSensorCritLow yadroTempSensorCritHigh yadroTempSensorState
     "ambient"           27.000 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
   "RTC_temp1"           21.000 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
 "INlet_Temp1"           21.000 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
 "INlet_Temp2"           20.500 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
"OUTlet_Temp1"           21.500 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
"OUTlet_Temp2"           22.000 °C               .000 °C              35.000 °C               .000 °C              40.000 °C               normal
...
```

All exporting data may be viewed by command `snmpwalk`:
```shell
$ snmpwalk -v2c -cpublic  manzoni.dev.yadro.com YADRO-MIB::yadroTempSensorsTable
YADRO-MIB::yadroTempSensorValue."ambient" = INTEGER: 26.750 °C
YADRO-MIB::yadroTempSensorValue."RTC_temp1" = INTEGER: 20.750 °C
YADRO-MIB::yadroTempSensorValue."INlet_Temp1" = INTEGER: 21.500 °C
YADRO-MIB::yadroTempSensorValue."INlet_Temp2" = INTEGER: 20.500 °C
YADRO-MIB::yadroTempSensorValue."OUTlet_Temp1" = INTEGER: 22.000 °C
YADRO-MIB::yadroTempSensorValue."OUTlet_Temp2" = INTEGER: 22.000 °C
YADRO-MIB::yadroTempSensorWarnLow."ambient" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnLow."RTC_temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnLow."INlet_Temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnLow."INlet_Temp2" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnLow."OUTlet_Temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnLow."OUTlet_Temp2" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorWarnHigh."ambient" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorWarnHigh."RTC_temp1" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorWarnHigh."INlet_Temp1" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorWarnHigh."INlet_Temp2" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorWarnHigh."OUTlet_Temp1" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorWarnHigh."OUTlet_Temp2" = INTEGER: 35.000 °C
YADRO-MIB::yadroTempSensorCritLow."ambient" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritLow."RTC_temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritLow."INlet_Temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritLow."INlet_Temp2" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritLow."OUTlet_Temp1" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritLow."OUTlet_Temp2" = INTEGER: .000 °C
YADRO-MIB::yadroTempSensorCritHigh."ambient" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorCritHigh."RTC_temp1" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorCritHigh."INlet_Temp1" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorCritHigh."INlet_Temp2" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorCritHigh."OUTlet_Temp1" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorCritHigh."OUTlet_Temp2" = INTEGER: 40.000 °C
YADRO-MIB::yadroTempSensorState."ambient" = INTEGER: normal(1)
YADRO-MIB::yadroTempSensorState."RTC_temp1" = INTEGER: normal(1)
YADRO-MIB::yadroTempSensorState."INlet_Temp1" = INTEGER: normal(1)
YADRO-MIB::yadroTempSensorState."INlet_Temp2" = INTEGER: normal(1)
YADRO-MIB::yadroTempSensorState."OUTlet_Temp1" = INTEGER: normal(1)
YADRO-MIB::yadroTempSensorState."OUTlet_Temp2" = INTEGER: normal(1)
...
```

### SNMPv3 support

For manage SNMPv3 access in runtime required `snmpusm` and `snmpvacm` tools from `net-snmp` package.

I did create the snmp user `root` with password `0penBmcAA` for control snmpd.
But by default him have access only from localhost.
For allowing access from other hosts, you should add/modify access rule in snmpd.conf:
```shell
...
com2sec readwrite  <Your host>  private
...
```

New user can be created over 3 steps:
1. Create copy of existen user
```shell
$ snmpusm -v3 -uroot -a0penBmcAA -x0penBmcAA -lauthPriv <OpenBMC-Host> create user01
```

2. Change password for new user:
```shell
$ snmpusm -v3 -uroot -a0penBmcAA -x0penBmcAA -lauthPriv <OpenBMC-Host> passwd 0penBmcAA NewPassword
```

3. Allow for new user read our data over snmp (add user to group MyRoGroup):
```shell
$ snmpvacm -v3 -uroot -a0penBmcAA -x0penBmc -lauthPriv <OpenBMC-Host> createSec2Group 3 user01 MyRoGroup
```

All users present in `SNMP-USER-BASED-SM-MIB::usmUserTable`.

### SNMP traps

For receive SNMP traps you should add receivers to snmpd.conf
```shell
trap2sink <receiver-host> <receiver-community>
```

## snmpcfg

This is a DBus service with interface `xyz.openbmc_project.SNMPCfg` 
and object path `/xyz/openbmc_project/snmpcfg` for manage snmpd.conf content.

There has three methods:
- GetConfig - return body of actual snmpd.conf
- SetConfig - replace body of snmpd.conf with specified data and restart snmpd.
- ResetConfig - restore default version of snmpd.conf and restart snmpd.

This subproject will be moved to separate project in fututre.
