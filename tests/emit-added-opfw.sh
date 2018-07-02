#!/bin/sh

gdbus emit --system --object-path '/xyz/openbmc_project/inventory'          \
           --signal org.freedesktop.DBus.ObjectManager.InterfacesAdded      \
           'objectpath "/xyz/openbmc_project/inventory/system/chassis/motherboard/opfw"' \
           "{                                                               \
           'org.freedesktop.DBus.Peer': @a{sv} {},                          \
           'org.freedesktop.DBus.Introspectable': @a{sv} {},                \
           'org.freedesktop.DBus.Properties': @a{sv} {},                    \
           'xyz.openbmc_project.Inventory.Opfw': {                          \
                'BuildrootVersion':<'buildroot-2018.02.2-7-gcb36c6d'>,      \
                'SkibootVersion':  <'skiboot-v6.0.1-27-g34e9c3c1edb3-p13e1584'>, \
                'HostbootVersion': <'hostboot-p8-d3025f5-pe7d78e0'>,         \
                'LinuxVersion':    <'occ-p8-28f2cec-pf0b771d'>,                 \
                'PetitbootVersion':<'linux-4.16.13-openpower1-p908fb4b'>,   \
                'MachinexmlVersion':<'petitboot-1.8.0'>                     \
           },                                                               \
           'xyz.openbmc_project.Inventory.Item': {                          \
                'PrettyName':<'OpenPOWER Firmware'>,                        \
                'Present':<true>                                            \
           },                                                               \
           'xyz.openbmc_project.Inventory.Decorator.Revision': {            \
                'Version':<'open-power-vesnin-v2.0-45-g16f9312'>            \
           }                                                                \
           }"

