#!/bin/sh

gdbus emit --system --object-path '/xyz/openbmc_project/inventory'          \
           --signal org.freedesktop.DBus.ObjectManager.InterfacesAdded      \
           'objectpath "/xyz/openbmc_project/inventory/system/chassis"'     \
           "{                                                               \
           'org.freedesktop.DBus.Peer': @a{sv} {},                          \
           'org.freedesktop.DBus.Introspectable': @a{sv} {},                \
           'org.freedesktop.DBus.Properties': @a{sv} {},                    \
           'xyz.openbmc_project.Inventory.Decorator.CoolingType': {         \
                'AirCooled': <true>, 'WaterCooled': <false>                 \
           }                                                                \
           }"

