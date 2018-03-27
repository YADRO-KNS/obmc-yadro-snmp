#!/bin/sh

gdbus emit --system --object-path '/xyz/openbmc_project/sensors'            \
           --signal org.freedesktop.DBus.ObjectManager.InterfacesAdded      \
           'objectpath "/xyz/openbmc_project/sensors/temperature/MEMBUF1"'  \
           "{                                                               \
           'org.freedesktop.DBus.Peer': @a{sv} {},                          \
           'org.freedesktop.DBus.Introspectable': @a{sv} {},                \
           'org.freedesktop.DBus.Properties': @a{sv} {},                    \
           'org.freedesktop.DBus.ObjectManager': @a{sv} {},                 \
           'xyz.openbmc_project.Sensor.Threshold.Critical': {               \
                'CriticalHigh':<int64 83000>,                               \
                'CriticalLow':<int64 0>,                                    \
                'CriticalAlarmHigh':<false>,                                \
                'CriticalAlarmLow':<false>                                  \
           },                                                               \
           'xyz.openbmc_project.Sensor.Threshold.Warning': {                \
                'WarningHigh':<int64 78000>,                                \
                'WarningLow':<int64 0>,                                     \
                'WarningAlarmHigh':<false>,                                 \
                'WarningAlarmLow':<false>                                   \
           },                                                               \
           'xyz.openbmc_project.Sensor.Value': {                            \
                'Value':<int64 48000>,                                      \
                'Unit':<'xyz.openbmc_project.Sensor.Value.Unit.DegreesC'>,  \
                'Scale':<int64 -3>                                          \
           }                                                                \
           }"

