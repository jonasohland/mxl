#include "Protocol.hpp"
#include <cassert>
#include <memory>
#include "DataLayout.hpp"
#include "Domain.hpp"
#include "Exception.hpp"
#include "ProtocolEgress.hpp"
#include "ProtocolIngress.hpp"

namespace mxl::lib::fabrics::ofi
{
    std::unique_ptr<IngressProtocol> selectProtocol(std::shared_ptr<Domain> domain, DataLayout const& layout, std::vector<Region> const& dstRegions)
    {
        if (layout.isVideo())
        {
            auto proto = std::make_unique<IngressProtocolWriter>();
            domain->registerRegions(dstRegions, FI_REMOTE_WRITE);

            return proto;
        }
        else if (layout.isAudio()) // audio
        {
            // Continuous flow should only have a single region
            assert(dstRegions.size() == 1);
            return std::make_unique<IngressProtocolWriterWithBounceBuffer>(std::move(domain), layout.asAudio(), dstRegions.front());
        }
        throw Exception::invalidArgument("Unsupported data layout for ingress protocol selection.");
    }

    std::unique_ptr<EgressProtocol> selectProtocol(Endpoint& ep, DataLayout const& layout, TargetInfo const& info)
    {
        if (layout.isVideo())
        {
            return std::make_unique<EgressProtocolWriterDiscrete>(ep, info.remoteRegions, layout.asVideo());
        }
        else if (layout.isAudio()) // audio
        {
            return std::make_unique<EgressProtocolWriterContinuous>(ep, info.remoteRegions, layout.asAudio());
        }
        throw Exception::invalidArgument("Unsupported data layout for egress protocol selection.");
    }

}
