pkglib_LTLIBRARIES = yadro.la

yadro_la_SOURCES = \
		yadro.cpp \
		yadro_sensorstables.cpp \
		sensors.cpp \
		watcher.cpp

yadro_la_CXXFLAGS = -I$(top_srcdir)/src $(SDBUSPLUS_CFLAGS)
yadro_la_LDFLAGS  = -module -avoid-version -shared -fPIC $(SDBUSPLUS_LIBS)

