#!/bin/sh

gdbus emit --system                                                         \
           --object-path '/org/openbmc/control/power0'                      \
           --signal 'org.freedesktop.DBus.Properties.PropertiesChanged'     \
           'org.openbmc.control.Power' "{'state':<int32 ${1:-0}>}" '@as []'

