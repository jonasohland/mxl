// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <utility>

namespace mxl::lib
{
    template<typename F>
    class Deferred
    {
    public:
        constexpr ~Deferred() noexcept(std::is_nothrow_invocable_v<F>)
        {
            _f();
        }

    private:
        constexpr Deferred(F&& f)
            : _f(std::forward<F>(f))
        {}

        template<typename N>
        friend constexpr Deferred<N> defer(N&& f);

    private:
        F _f;
    };

    template<typename F>
    constexpr Deferred<F> defer(F&& f)
    {
        return Deferred{std::forward<F>(f)};
    }
}
