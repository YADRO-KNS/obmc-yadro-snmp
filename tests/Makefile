CXX=c++ -g
CXXFLAGS=-I. -I../yadro
LDFLAGS=-lsdbusplus -lsystemd

SOURCE_FILES = watcher-test.cpp 		\
			   ../yadro/watcher.cpp 	\
			   ../yadro/versions.cpp 	\
			   ../yadro/sensors.cpp 	

OBJECT_FILES = $(SOURCE_FILES:.cpp=.o)

all: watcher

watcher: $(OBJECT_FILES)
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	-rm -fr $(OBJECT_FILES) watcher

