#!/bin/sh

gdbus emit --system                                                         \
           --object-path '/xyz/openbmc_project/sensors/temperature/ambient' \
           --signal 'org.freedesktop.DBus.Properties.PropertiesChanged'     \
           'xyz.openbmc_project.Sensor.Value' "{'Value':<int64 34250>}"     \
           '@as []'

