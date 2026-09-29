// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "VerbsDevice.hpp"
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <memory>
#include <span>
#include <infiniband/verbs.h>
#include <mxl-internal/Logging.hpp>

namespace mxl::lib::fabrics::ofi
{
    namespace
    {
        /** \brief Deleter that releases a list returned by ibv_get_device_list(). */
        struct DeviceListDeleter
        {
            /** \brief Free the device list. */
            void operator()(::ibv_device** list) const noexcept
            {
                ::ibv_free_device_list(list);
            }
        };

        /** \brief Deleter that closes a device context returned by ibv_open_device(). */
        struct ContextDeleter
        {
            /** \brief Close the device context. */
            void operator()(::ibv_context* context) const noexcept
            {
                ::ibv_close_device(context);
            }
        };
    }

    std::optional<std::size_t> queryVerbsMaxSge(std::string const& deviceName)
    {
        auto deviceCount = 0;
        auto const deviceList = std::unique_ptr<::ibv_device*, DeviceListDeleter>{::ibv_get_device_list(&deviceCount)};
        if (!deviceList)
        {
            MXL_WARN("Failed to get the ibverbs device list: {}", std::strerror(errno));
            return std::nullopt;
        }

        auto const devices = std::span{deviceList.get(), static_cast<std::size_t>(deviceCount)};
        auto const device = std::ranges::find_if(devices, [&](::ibv_device* dev) { return deviceName == ::ibv_get_device_name(dev); });
        if (device == devices.end())
        {
            MXL_WARN("ibverbs device '{}' not found", deviceName);
            return std::nullopt;
        }

        auto const context = std::unique_ptr<::ibv_context, ContextDeleter>{::ibv_open_device(*device)};
        if (!context)
        {
            MXL_WARN("Failed to open ibverbs device '{}': {}", deviceName, std::strerror(errno));
            return std::nullopt;
        }

        auto attr = ::ibv_device_attr{};
        if (auto const ret = ::ibv_query_device(context.get(), &attr); ret != 0)
        {
            MXL_WARN("Failed to query ibverbs device '{}': {}", deviceName, std::strerror(ret));
            return std::nullopt;
        }

        if (attr.max_sge <= 0)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(attr.max_sge);
    }
}
