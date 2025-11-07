// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#include "mxl-internal/MediaUtils.hpp"
#include <mxl/platform.h>

MXL_EXPORT
std::uint32_t mxl::lib::getV210LineLength(std::size_t width)
{
    return static_cast<std::uint32_t>((width + 47) / 48 * 128);
}

MXL_EXPORT
std::uint32_t mxl::lib::get10BitAlphaLineLength(std::size_t width)
{
    return static_cast<std::uint32_t>((width + 2) / 3 * 4);
}
