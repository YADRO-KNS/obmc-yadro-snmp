/**
 * Compile: c++ -DTEST -lsdbusplus -lsystem sensors.cpp watcher.cpp \
 *          watcher-test.cpp -o watcher
 */
#include <sensors.hpp>
#include <watcher.hpp>

int main(int argc, char* argv[])
{
    initialize_sensors();

    dbuswatcher w(argc > 1 ? argv[1] : nullptr);
    w.run();

    return 0;
}
