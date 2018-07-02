#!/bin/sh

gdbus emit --system --object-path '/xyz/openbmc_project/sensors'            \
           --signal org.freedesktop.DBus.ObjectManager.InterfacesRemoved    \
           'objectpath "/xyz/openbmc_project/sensors/temperature/ambient"'  \
           "[                                                               \
           'org.freedesktop.DBus.Peer',                                     \
           'org.freedesktop.DBus.Introspectable',                           \
           'org.freedesktop.DBus.Properties',                               \
           'xyz.openbmc_project.Sensor.Value'                               \
           ]"

