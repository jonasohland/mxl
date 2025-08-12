#pragma once

#include <map>
#include <optional>
#include <string>
#include <fmt/base.h>
#include <fmt/format.h>
#include <rdma/fabric.h>
#include <string_view>
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    class Provider final
    {
        enum class Value
        {
            TCP = 0,
            VERBS = 1,
            EFA = 2,
            SHM = 3,
        };
        static std::map<std::string_view, Value> const _stringMap;

    public:
        [[nodiscard]]
        mxlFabricsProvider toAPI() const noexcept;
        static std::optional<Provider> fromAPI(mxlFabricsProvider api) noexcept;

        static std::optional<Provider> fromString(std::string const& s) noexcept;

        [[nodiscard]]
        ::fi_ep_type endpointType() const noexcept;

    private:
        friend fmt::formatter<Provider>;

        explicit Provider(Value value) noexcept
            : _inner(value)
        {}

        Value _inner;
    };

}

namespace ofi = mxl::lib::fabrics::ofi;

template<>
struct fmt::formatter<ofi::Provider>
{
    constexpr auto parse(fmt::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename Context>
    constexpr auto format(ofi::Provider const& provider, Context& ctx) const
    {
        switch (provider._inner)
        {
            case ofi::Provider::Value::TCP:   return fmt::format_to(ctx.out(), "tcp");
            case ofi::Provider::Value::VERBS: return fmt::format_to(ctx.out(), "verbs");
            case ofi::Provider::Value::EFA:   return fmt::format_to(ctx.out(), "efa");
            case ofi::Provider::Value::SHM:   return fmt::format_to(ctx.out(), "shm");
            default:                          return fmt::format_to(ctx.out(), "unknown");
        }
    }
};
