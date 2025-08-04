#include "Provider.hpp"
#include <map>
#include <optional>
#include <string_view>

namespace mxl::lib::fabrics::ofi
{

    std::map<std::string_view, Provider::Value> const Provider::_stringMap = {
        {"tcp",   Provider::Value::TCP  },
        {"verbs", Provider::Value::VERBS},
        {"efa",   Provider::Value::EFA  },
        {"shm",   Provider::Value::SHM  },
    };

    mxlFabricsProvider Provider::toAPI() const noexcept
    {
        switch (_inner)
        {
            case Value::TCP:   return MXL_SHARING_PROVIDER_TCP;
            case Value::VERBS: return MXL_SHARING_PROVIDER_VERBS;
            case Value::EFA:   return MXL_SHARING_PROVIDER_EFA;
            case Value::SHM:   return MXL_SHARING_PROVIDER_SHM;
        }
        return MXL_SHARING_PROVIDER_AUTO;
    }

    std::optional<Provider> Provider::fromAPI(mxlFabricsProvider api) noexcept
    {
        switch (api)
        {
            case MXL_SHARING_PROVIDER_AUTO:
            case MXL_SHARING_PROVIDER_TCP:   return Provider{Value::TCP};
            case MXL_SHARING_PROVIDER_VERBS: return Provider{Value::VERBS};
            case MXL_SHARING_PROVIDER_EFA:   return Provider{Value::EFA};
            case MXL_SHARING_PROVIDER_SHM:   return Provider{Value::SHM};
        }

        return std::nullopt;
    }

    std::optional<Provider> Provider::fromString(std::string const& s) noexcept
    {
        auto it = _stringMap.find(s);
        if (it != _stringMap.end())
        {
            return Provider{it->second};
        }
        return std::nullopt;
    }

    ::fi_ep_type Provider::endpointType() const noexcept
    {
        switch (_inner)
        {
            case Value::TCP:   return FI_EP_MSG;
            case Value::VERBS: return FI_EP_MSG;
            case Value::EFA:   return FI_EP_RDM;
            case Value::SHM:   return FI_EP_RDM;
        }
        return FI_EP_RDM; // Default to RDM for SHM and unknown providers
    }

}
