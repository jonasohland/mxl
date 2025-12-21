// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace mxl::lib
{
    template<typename To, typename From>
    std::unique_ptr<To> dynamic_pointer_cast(std::unique_ptr<From>&& source) noexcept
    {
        auto const p = dynamic_cast<To*>(source.get());
        if (p != nullptr)
        {
            source.release();
        }
        return std::unique_ptr<To>{p};
    }
}
