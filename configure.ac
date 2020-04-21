AC_PREREQ([2.69])
AC_INIT([yadro-snmp], 
        [m4_esyscmd_s([git describe --dirty --long --always])], 
        [https://github.com/YADRO-KNS/yadro-snmp/issues])
AC_LANG([C++])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P

# Checks for typedefs, structures and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

# Checks for libraries.
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,\
    AC_MSG_ERROR(["Required sdbusplus package."]))
AC_SUBST(SDBUSPLUS_CFLAGS)
AC_SUBST(SDBUSPLUS_LIBS)

# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

AC_CHECK_HEADERS([net-snmp/net-snmp-config.h],,\
    AC_MSG_ERROR(["Requires net-snmp headers"]))

# Check for net-snmp-config
AC_PATH_PROG([NETSNMP_CONFIG], [net-snmp-config])
AS_IF([test "x$NETSNMP_CONFIG" == "x"],
    AC_MSG_ERROR(["Requires net-snmp-config"]))

NETSNMP_CFLAGS=`$NETSNMP_CONFIG --base-cflags`
NETSNMP_AGENT_LIBS=`$NETSNMP_CONFIG --agent-libs`
AC_SUBST([NETSNMP_CFLAGS])
AC_SUBST([NETSNMP_AGENT_LIBS])

# Checks for library functions
LT_INIT # Required for systemd linking

PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],
     [],
     [with_systemdsystemunitdir=auto]
)
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"],
    [def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
           [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                  [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])]
            )
            with_systemdsystemunitdir=no],
           [with_systemdsystemunitdir="$def_systemdsystemunitdir"]
     )]
)
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])]
)
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

AC_ARG_ENABLE([agent],
    AS_HELP_STRING([--disable-agent], [Disable yadro-snmp-agent.]))
AC_ARG_ENABLE([cfg-manager],
    AS_HELP_STRING([--disable-cfg-manager], [Disable yadro-snmp-cfg-manager.]))

AM_CONDITIONAL([WANT_AGENT], [test "x$enable_agent" != "xno"])
AM_CONDITIONAL([WANT_CFG_MANAGER], [test "x$enable_cfg_manager" != "xno"])

AC_ARG_VAR(HOST_INSTANCE, "OpenBMC host instance")
AS_IF([test "x$HOST_INSTANCE" = "x"], [HOST_INSTANCE="0"])

AC_ARG_VAR(SYSTEMD_TARGET, "Target for starting this service")
AS_IF([test "x$SYSTEMD_TARGET" = "x"], [SYSTEMD_TARGET="multi-user.target"])

# Package specific checks.
AS_IF([test "x$enable_agent" != "xno"], [
       AC_CONFIG_FILES([agent/Makefile])
       AC_CONFIG_FILES([agent/yadro-snmp-agent.service])
])
AS_IF([test "x$enable_cfg_manager" != "xno"], [
       AC_CONFIG_FILES([snmpcfg/Makefile])
       AC_CONFIG_FILES([snmpcfg/yadro-snmp-cfg-manager.service])
])

# Create configured output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

