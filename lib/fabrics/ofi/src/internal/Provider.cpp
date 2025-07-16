#include "Provider.hpp"
#include <stdexcept>

namespace mxl::lib::fabrics::ofi
{

    Provider providerFromAPI(mxlFabricsProvider api)
    {
        switch (api)
        {
            case MXL_SHARING_PROVIDER_AUTO:
            case MXL_SHARING_PROVIDER_TCP:   return Provider::TCP;
            case MXL_SHARING_PROVIDER_VERBS: return Provider::VERBS;
            case MXL_SHARING_PROVIDER_EFA:   return Provider::EFA;
            default:                         throw std::invalid_argument("Unknown provider");
        }
    }
}
