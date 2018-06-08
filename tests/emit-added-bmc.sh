#!/bin/sh

gdbus emit --system --object-path '/xyz/openbmc_project/software'           \
           --signal org.freedesktop.DBus.ObjectManager.InterfacesAdded      \
           'objectpath "/xyz/openbmc_project/software/1f6027c9"'            \
           "{                                                               \
           'org.freedesktop.DBus.Peer': @a{sv} {},                          \
           'org.freedesktop.DBus.Introspectable': @a{sv} {},                \
           'org.freedesktop.DBus.Properties': @a{sv} {},                    \
           'xyz.openbmc_project.Common.FilePath': {                         \
                'Path':<'/tmp/images/1f6027c9'>                             \
           },                                                               \
           'xyz.openbmc_project.Software.Version': {                        \
                'Version':<'v2.2-441-gc04c198-dirty'>,                      \
                'Purpose':<'xyz.openbmc_project.Software.Version.VersionPurpose.BMC'> \
           },                                                               \
           'xyz.openbmc_project.Software.Activation': {                     \
                'Activation':<'xyz.openbmc_project.Software.Activation.Activations.Ready'>, \
                'RequestedActivation':<'xyz.openbmc_project.Software.Activation.RequestedActivations.None'> \
           }
           }"


#            'org.openbmc.Associations': {                                    \
#                 'associations':<struct {'inventory', 'activation', ''} >    \
#            }                                                                \
