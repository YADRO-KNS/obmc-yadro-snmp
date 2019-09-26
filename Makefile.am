SUBDIRS = .

if WANT_AGENT
SUBDIRS += agent
endif
if WANT_CFG_MANAGER
SUBDIRS += snmpcfg
endif

mibsdir = $(datarootdir)/snmp/mibs
mibs_DATA = mibs/YADRO-MIB.txt
