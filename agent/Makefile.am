AM_CPPFLAGS = -iquote $(top_srcdir)

bin_PROGRAMS = yadro-snmp-agent

yadro_snmp_agent_SOURCES = 		\
		snmp.cpp 				\
		yadro/powerstate.cpp 	\
		yadro/sensors.cpp 		\
		yadro/software.cpp 		\
		yadro/inventory.cpp 	\
		main.cpp

yadro_snmp_agent_CXXFLAGS = $(SDBUSPLUS_CFLAGS) $(NETSNMP_CFLAGS)
yadro_snmp_agent_LDADD = $(SDBUSPLUS_LIBS) $(NETSNMP_AGENT_LIBS)

if HAVE_SYSTEMD
systemdsystemunit_DATA = yadro-snmp-agent.service
endif
