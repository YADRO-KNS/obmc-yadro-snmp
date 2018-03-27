# Be sure to build needed files before compiling
BUILT_SOURCES = snmpd.0.conf 

CLEANFILES=${BUILT_SOURCES}

snmpconfdir=$(sysconfdir)/snmp
snmpconf_DATA = snmpd.conf 

snmpd.0.conf:
	@echo "createUser $(SNMP_USERNAME) MD5 $(SNMP_PASSWORD) DES" > snmpd.0.conf

snmpvardir=$(localstatedir)/lib/net-snmp
snmpvar_DATA = snmpd.0.conf

install-data-hook:
	chmod 0700 $(DESTDIR)$(localstatedir)/lib/net-snmp
	chmod 0600 $(DESTDIR)$(localstatedir)/lib/net-snmp/snmpd.0.conf
