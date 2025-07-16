#pragma once

#include "mxl/fabrics.h"

namespace mxl::lib::fabrics::ofi
{
    enum class Provider
    {
        TCP,
        VERBS,
        EFA,
    };

    Provider providerFromAPI(mxlFabricsProvider api);
}