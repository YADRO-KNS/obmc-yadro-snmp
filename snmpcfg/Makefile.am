sbin_PROGRAMS = yadro-snmpcfg 

nobase_nodist_include_HEADERS = \
	xyz/openbmc_project/SNMPCfg/server.hpp

yadro_snmpcfg_SOURCES = \
	snmpcfg-server.cpp \
	xyz/openbmc_project/SNMPCfg/server.cpp 

yadro_snmpcfg_CXXFLAGS = $(SDBUSPLUS_CFLAGS)
yadro_snmpcfg_LDADD = $(SDBUSPLUS_LIBS)

# Be sure to build needed files before compiling
BUILT_SOURCES = \
	xyz/openbmc_project/SNMPCfg/server.cpp \
	xyz/openbmc_project/SNMPCfg/server.hpp 

CLEANFILES=${BUILT_SOURCES}

xyz/openbmc_project/SNMPCfg/server.cpp: \
xyz/openbmc_project/SNMPCfg.interface.yaml \
xyz/openbmc_project/SNMPCfg/server.hpp
	@mkdir -p $(@D)
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-cpp \
xyz.openbmc_project.SNMPCfg > $@

xyz/openbmc_project/SNMPCfg/server.hpp: \
xyz/openbmc_project/SNMPCfg.interface.yaml
	@mkdir -p $(@D)
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-header \
xyz.openbmc_project.SNMPCfg > $@


