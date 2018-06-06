/**
 * @brief sd_event_source C++ wrapper
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
 */
#pragma once

#include <memory>
#include <systemd/sd-event.h>

namespace sdevent
{
namespace source
{

namespace details
{

/** @brief unique_ptr functor to release a source reference. */
struct SourceDeleter
{
    void operator()(sd_event_source* ptr) const
    {
        deleter(ptr);
    }

    decltype(&sd_event_source_unref) deleter = sd_event_source_unref;
};

/* @brief Alias 'Source' to a unique_ptr type for auto-release. */
using Source = std::unique_ptr<sd_event_source, SourceDeleter>;

} // namespace details

/** @class Source
 *  @brief Provides C++ bindings to the sd_event_source* functions.
 */
class Source
{
  public:
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    Source() = delete;
    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;
    Source(Source&&) = default;
    Source& operator=(Source&&) = default;
    ~Source() = default;

    /** @brief Conversion constructor from 'sd_event_source*'.
     *
     *  Increments ref-count of the source-pointer and releases it
     *  when done.
     */
    explicit Source(sd_event_source* s) : src(sd_event_source_ref(s))
    {
    }

    /** @brief Constructor for 'source'.
     *
     *  Takes ownership of the source-pointer and releases it when done.
     */
    Source(sd_event_source* s, std::false_type) : src(s)
    {
    }

    /** @brief Check if source contains a real pointer. (non-nullptr). */
    explicit operator bool() const
    {
        return bool(src);
    }

    /** @brief Test whether or not the source can generate events. */
    auto enabled()
    {
        int enabled;
        sd_event_source_get_enabled(src.get(), &enabled);
        return enabled;
    }

    /** @brief Allow the source to generate events. */
    void enable(int enable)
    {
        sd_event_source_set_enabled(src.get(), enable);
    }

  private:
    details::Source src;
};
} // namespace source
} // namespace sdevent
