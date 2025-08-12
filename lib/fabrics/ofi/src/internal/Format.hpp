#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
#include <mxl/fabrics.h>

template<>
struct fmt::formatter<mxlFabricsProvider>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename Context>
    constexpr auto format(mxlFabricsProvider const& provider, Context& ctx) const
    {
        switch (provider)
        {
            case MXL_SHARING_PROVIDER_AUTO:  return fmt::format_to(ctx.out(), "auto");
            case MXL_SHARING_PROVIDER_TCP:   return fmt::format_to(ctx.out(), "tcp");
            case MXL_SHARING_PROVIDER_VERBS: return fmt::format_to(ctx.out(), "verbs");
            case MXL_SHARING_PROVIDER_EFA:   return fmt::format_to(ctx.out(), "efa");
            case MXL_SHARING_PROVIDER_SHM:   return fmt::format_to(ctx.out(), "shm");
            default:                         return fmt::format_to(ctx.out(), "unknown");
        }
    }
};

namespace mxl::lib::fabrics::ofi

{
    std::string fiProtocolToString(uint64_t) noexcept;
}
