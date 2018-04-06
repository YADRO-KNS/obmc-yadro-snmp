/**
 * @brief DBus objects watcher declaration
 *
 * This file is part of phosphor-snmp project.
 *
 * Copyright (c) 2018 YADRO
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
#include "sdbus-helper.hpp"                 // sdbusplus::helper::helper
#include "sensors.hpp"                      // sensor_t
#include "versions.hpp"                     // versionBMC, versionHFW

class dbuswatcher : public sdbusplus::helper::helper
{
        // Internal types

        using match_t = sdbusplus::bus::match::match;
        using matches_arr_t = std::vector<match_t>;
        using matches_map_t = std::unordered_map<std::string, match_t>;

    public:

        dbuswatcher(const char* host = nullptr);

        // API

        void            updatePowerState(void);
        void            updateSensors(void);
        void            updateBMCVersion(void);
        void            updateHFWVersion(void);
        void            run(void);

    protected:
        //

        virtual void    sensorChangeValue(sensor_t* sensor, int prev);
        virtual void    sensorChangeState(sensor_t* sensor,
                                          const std::string& type,
                                          sensor_t::state_t prev);
        virtual void    powerStateChanged(int prev);
        virtual void    versionBMCChanged(const std::string& prev);
        virtual void    versionHFWChanged(const std::string& prev);

    private:
        // Tools

        bool            splitObjectPath(const std::string& path,
                                        std::string& name,
                                        std::string& type) const;
        void            getSensorValues(const std::string& object,
                                        const std::string& path);
        void            setBMCVersion(const std::string& ver);
        void            setHFWVersion(const std::string& ver);

        void            onSensorsAdded(sdbusplus::message::message& m);
        void            onSensorsRemoved(sdbusplus::message::message& m);
        void            onPropertiesChanged(sensor_t* s,
                                            const std::string& type,
                                            int scale,
                                            sdbusplus::message::message& m);
        void            onPowerStateChanged(sdbusplus::message::message& m);

    private:
        // Member variables

        matches_arr_t m_staticMatches;      //!< Satic match rules.
        matches_map_t m_sensorsMatches;     //!< Collection of match rules for
                                            //!  `PropertiesChanged` signals for
                                            //!  each active sensor
};
