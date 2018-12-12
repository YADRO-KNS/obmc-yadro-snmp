/**
 * @brief Export DBus property to scalar MIB value
 *
 * This file is part of yadro-snmp project.
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
 */
#pragma once

#include "sdbusplus/helper.hpp"
#include <memory>

namespace phosphor
{
namespace snmp
{
namespace data
{
template <typename T> class Scalar
{
  public:
    using value_t = sdbusplus::message::variant<T>;

    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    Scalar() = delete;
    Scalar(const Scalar&) = delete;
    Scalar& operator=(const Scalar&) = delete;
    Scalar(Scalar&&) = default;
    Scalar& operator=(Scalar&&) = default;
    ~Scalar() = default;

    /**
     * @brief Object constructor
     */
    Scalar(const std::string& busName, const std::string& path,
           const std::string& iface, const std::string& prop,
           const T& initValue) :
        _value(initValue),
        _busName(busName), _path(path), _iface(iface), _prop(prop),
        _onChangedMatch(sdbusplus::helper::helper::getBus(),
                        sdbusplus::bus::match::rules::propertiesChanged(
                            path.c_str(), iface.c_str()),
                        std::bind(&Scalar<T>::onPropertyChanged, this,
                                  std::placeholders::_1))
    {
    }

    /**
     * @brief Sent request to DBus object and store new value of property
     */
    void update()
    {
        try
        {
            auto r = sdbusplus::helper::helper::callMethod(
                _busName, _path, sdbusplus::helper::PROPERTIES_IFACE, "Get",
                _iface, _prop);

            value_t var;
            r.read(var);
            setValue(var);
        }
        catch (const sdbusplus::exception::SdBusError&)
        {
            // Corresponding service is not started yet.
            // When service is started, we'll receive data with
            // `propertiesChanged` signal.
            // So, this catch block is just for silencing the exception.
        }
    }

    const T& getValue() const
    {
        return _value;
    }

    const std::string& getBusName() const
    {
        return _busName;
    }

    const std::string& getPath() const
    {
        return _path;
    }

    const std::string& getInterface() const
    {
        return _iface;
    }

    const std::string& getProperty() const
    {
        return _prop;
    }

  protected:
    /**
     * @brief DBus signal `PropertiesChanged` handler
     */
    void onPropertyChanged(sdbusplus::message::message& m)
    {
        std::string iface;
        std::map<std::string, value_t> data;
        std::vector<std::string> v;

        m.read(iface, data, v);

        if (data.find(_prop) != data.end())
        {
            setValue(data[_prop]);
        }
    }

    /**
     * @brief Setter for actual value
     */
    virtual void setValue(value_t& var)
    {
        auto newValue = sdbusplus::message::variant_ns::get<T>(var);
        std::swap(_value, newValue);
    }

  private:
    T _value;
    std::string _busName;
    std::string _path;
    std::string _iface;
    std::string _prop;

    sdbusplus::bus::match::match _onChangedMatch;
};

} // namespace data
} // namespace snmp
} // namespace phosphor
