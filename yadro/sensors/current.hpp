/**
 * This vector containt data for tests only.
 * For production version it should be genereated
 * at compile time by yaml files from hwmon project.
 */
sensors_arr_t currentSensors = {{
    {"PSU0_IIN", 0, 0, 10000, 0, 10000, sensor_t::E_DISABLED},
    {"PSU0_IOUT", 0, 0, 167000, 0, 167000, sensor_t::E_DISABLED},
    {"PSU1_IIN", 0, 0, 10000, 0, 10000, sensor_t::E_DISABLED},
    {"PSU1_IOUT", 0, 0, 167000, 0, 167000, sensor_t::E_DISABLED},
}};