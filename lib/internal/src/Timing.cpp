// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#include "mxl-internal/Timing.hpp"
#include <mxl/platform.h>
#include "mxl-internal/detail/ClockHelpers.hpp"

namespace mxl::lib
{
    MXL_EXPORT
    Timepoint currentTime(Clock clock) noexcept
    {
        auto result = Timepoint{};

        std::timespec ts;
        if (::clock_gettime(detail::clockToId(clock), &ts) == 0)
        {
            result = asTimepoint(ts) + detail::getClockOffset(clock);
        }
        return result;
    }

    Timepoint currentTimeUTC() noexcept
    {
        auto result = Timepoint{};

        std::timespec ts;
        if (::timespec_get(&ts, TIME_UTC) != 0)
        {
            result = asTimepoint(ts);
        }
        return result;
    }
}
