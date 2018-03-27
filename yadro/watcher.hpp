/**
 * @brief DBus objects watcher declaration
 *
 * This file is part of phosphor-snmp project.
 *
 * Copyright (c) 2018 YADRO (KNS Group LLC)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author: Alexander Filippov <a.filippov@yadro.com>
 */

#pragma once

#include <string>                           // std::string
#include <unordered_map>                    // std::unordered_map
#include <sdbusplus/bus.hpp>                // sdbusplus::bus::bus
#include <sdbusplus/bus/match.hpp>          // sdbusplus::bus::match::match
#include <sdbusplus/message.hpp>            // sdbusplus::message::message

#include "sensors.hpp"                      // sensor_t

class dbuswatcher
{
        // Internal types

        using match_t = sdbusplus::bus::match::match;
        using match_ptr = std::shared_ptr<match_t>;
        using matches_arr_t = std::unordered_map<std::string, match_ptr>;

    public:

        dbuswatcher(const char* host = nullptr);

        // API

        void            updatePowerState(void);
        void            updateSensors(void);
        void            run(void);
        void            terminate(void);

    protected:
        //

        virtual void    sensorChangeValue(sensor_t* sensor, int prev);
        virtual void    sensorChangeState(sensor_t* sensor,
                                          const std::string& type,
                                          sensor_t::state_t prev);
        virtual void    powerStateChanged(int prev);

    private:
        // Tools

        bool            splitObjectPath(const std::string& path,
                                        std::string& name,
                                        std::string& type) const;
        void            getSensorValues(const std::string& object,
                                        const std::string& path);

        void            onInterfacesAdded(sdbusplus::message::message& m);
        void            onPropertiesChanged(sensor_t* s,
                                            const std::string& type,
                                            int scale,
                                            sdbusplus::message::message& m);
        void            onPowerStateChanged(sdbusplus::message::message& m);

    private:
        // Member variables

        sdbusplus::bus::bus m_bus;          //!< DBus connection
        volatile bool m_running;            //!< Running flag
        match_ptr m_interfacesAddedMatch;   //!< InterfacesAdded signals match
        matches_arr_t m_sensorsMatches;     //!< Collection of PropertiesChanged matches
                                            //!  for each active sensor
        match_ptr m_powerStateMatch;        //!< PropertiesChanged signal match for power state
};

inline void dbuswatcher::terminate(void)
{
    m_running = false;
}
