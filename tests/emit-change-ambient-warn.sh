#!/bin/sh

gdbus emit --system                                                         \
           --object-path '/xyz/openbmc_project/sensors/temperature/ambient' \
           --signal 'org.freedesktop.DBus.Properties.PropertiesChanged'     \
           'xyz.openbmc_project.Sensor.Threshold.Warning'                   \
           "{'WarningAlarmHigh':<boolean ${1:-false}>}"                     \
           '@as []'

