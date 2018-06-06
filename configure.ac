AC_PREREQ([2.69])
AC_INIT([phosphor-snmp], 
        [m4_esyscmd_s([git describe --dirty --long --always])], 
        [https://github.com/YADRO-KNS/phosphor-snmp/issues])
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
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-fpic -Wall -Werror], [CXXFLAGS])

#LT_PREREQ([2.4.6])
#LT_INIT([dlopen disable-static])

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

AC_ARG_ENABLE([tracing],
    AS_HELP_STRING([--enable-tracing], [Enable trace messages (default no)]))
AS_IF([test "x$enable_tracing" == "xyes"], [CXXFLAGS="$CXXFLAGS -DWITH_TRACING"])

# Create SNMPv3 user for control snmpd in runtime
AC_ARG_VAR(SNMP_USERNAME, [SNMPv3 username for access to daemon in runtime.])
AC_ARG_VAR(SNMP_PASSWORD, [SNMPv3 password for access to daemon in runtime.])
AS_IF([test "x$SNMP_USERNAME" == "x"], [SNMP_USERNAME="root"])
AS_IF([test "x$SNMP_PASSWORD" == "x"], [SNMP_PASSWORD="0penBmcAA"])
AC_DEFINE_UNQUOTED([SNMP_USERNAME], ["$SNMP_USERNAME"], [SNMPv3 username.])
AC_DEFINE_UNQUOTED([SNMP_PASSWORD], ["$SNMP_PASSWORD"], [SNMPv3 password.])

# Create configured output
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile \
                 yadro/Makefile \
                 agent/Makefile \
                 snmpcfg/Makefile \
                 mibs/Makefile \
                 conf/Makefile])
AC_OUTPUT

