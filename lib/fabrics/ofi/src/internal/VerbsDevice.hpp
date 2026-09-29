// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace mxl::lib::fabrics::ofi
{
    /** \brief Query libibverbs for the maximum number of scatter-gather entries per work request of a device.
     *
     * The value is ibv_device_attr::max_sge. The libfabric verbs provider rejects a tx_attr->iov_limit hint that is larger
     * than this value, and returns at most FI_VERBS_TX_IOV_LIMIT (default 4) when no hint is given.
     *
     * \param deviceName The ibverbs device name. For the verbs provider with FI_EP_MSG endpoints this is fi_info::domain_attr::name
     * (for example "mlx5_0").
     * \return The maximum, or std::nullopt when the device is not found or cannot be queried.
     */
    [[nodiscard]]
    std::optional<std::size_t> queryVerbsMaxSge(std::string const& deviceName);
}
