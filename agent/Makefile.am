AM_CPPFLAGS = -iquote $(top_srcdir)

sbin_PROGRAMS = phosphor-snmp-agent

phosphor_snmp_agent_SOURCES = 		\
		snmp.cpp 					\
		main.cpp

phosphor_snmp_agent_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(NETSNMP_CFLAGS)
phosphor_snmp_agent_LDADD = $(SDBUSPLUS_LIBS) $(NETSNMP_AGENT_LIBS)

