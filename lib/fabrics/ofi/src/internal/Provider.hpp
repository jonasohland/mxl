#pragma once

#include <optional>
#include <string>
#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    enum class Provider
    {
        TCP,
        VERBS,
        EFA,
    };

    mxlFabricsProvider providerToAPI(Provider provider) noexcept;
    std::optional<Provider> providerFromAPI(mxlFabricsProvider api) noexcept;
    std::optional<Provider> providerFromString(std::string const& s) noexcept;
}
