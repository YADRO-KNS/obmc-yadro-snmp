/**
 * @brief sd_event C++ wrapper
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
#include <sdbusplus/bus.hpp>
#include <systemd/sd-event.h>
#include <csignal>

#include "sdevent/source.hpp"

#define WAIT_INFINITE -1

namespace sdevent
{
namespace event
{

namespace details
{

/** @brief unique_ptr functor to release an event reference. */
struct EventDeleter
{
    void operator()(sd_event* ptr) const
    {
        deleter(ptr);
    }

    decltype(&sd_event_unref) deleter = sd_event_unref;
};

/** @brief Alias 'Event' to a unique_ptr type for auto-release. */
using Event = std::unique_ptr<sd_event, EventDeleter>;

} // namespace details

/** @class Event
 *  @brief Provides C++ bindings to the sd_event_* class functions.
 */
class Event
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
    Event() = delete;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;
    ~Event() = default;

    /** @brief Conversion constructor from 'EventPtr'.
     *
     *  Increments ref-count of the event-pointer and releases it when
     *  done.
     */
    explicit Event(sd_event* e) : evt(sd_event_ref(e))
    {
    }

    /** @brief Constructor for 'Event'.
     *
     *  Takes ownership of the event-pointer and releases it when done.
     */
    Event(sd_event* e, std::false_type) : evt(e)
    {
    }

    /** @brief Release ownership of the stored event-pointer. */
    sd_event* release()
    {
        return evt.release();
    }

    /** @brief Wait indefinitely for new event sources. */
    int loop()
    {
        return sd_event_loop(evt.get());
    }

    /**
     * @brief Run single iteration of the event loop.
     *
     * @param usec - Maximum time (in mictoseconds) to wait for an event.
     *               (uint64_t)-1 to specify an infinite timeout.
     *
     * @return Negative errno-style error code if failure,
     *         Zero if timeout reached and positive value if an event was
     *         dispatched.
     */
    int run(uint64_t usec = WAIT_INFINITE)
    {
        return sd_event_run(evt.get(), usec);
    }

    /** @brief Attach to a DBus loop. */
    void attach(sdbusplus::bus::bus& bus)
    {
        bus.attach_event(evt.get(), SD_EVENT_PRIORITY_NORMAL);
    }

    /**
     * @brief Add a new I/O event source to an event loop.
     *
     * @param fd - file descriptor
     * @param events - bit mask of event to watch
     * @param cb - reference a function to call when the event source is
     *             triggered
     * @param userdata - userdata pointer will be passed to the handler function
     *
     * @return `sdevent::source::source` object
     */
    sdevent::source::Source add_io(int fd, uint32_t events,
                                   sd_event_io_handler_t cb,
                                   void* userdata = nullptr)
    {
        sd_event_source* src = nullptr;
        sd_event_add_io(evt.get(), &src, fd, events, cb, userdata);
        return sdevent::source::Source(src, std::false_type());
    }

    /**
     * @brief Add a new UNIX process signal event source to an event loop.
     *
     * @param signal - numeric signal to be handled
     * @param cb - reference a function to call when the event source is
     *             triggered. If nullptr specified, a default handler which
     *             casuses the program to exit cleanly will be used.
     * @param userdata - userdata pointer will be passed to the handler function
     *
     * @return `sdevent::source::source` object
     */
    sdevent::source::Source add_signal(int signal, sd_event_signal_handler_t cb,
                                       void* userdata = nullptr)
    {
        sd_event_source* src = nullptr;
        sd_event_add_signal(evt.get(), (cb ? &src : nullptr), signal, cb,
                            userdata);
        return sdevent::source::Source(src, std::false_type());
    }

  private:
    details::Event evt;
};

inline Event new_default()
{
    sd_event* evt = nullptr;
    sd_event_default(&evt);
    return Event(evt, std::false_type());
}

inline auto& get_default()
{
    static auto evt = new_default();
    return evt;
}

} // namespace event
} // namespace sdevent
