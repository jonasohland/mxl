#include "Protocol.hpp"
#include <cassert>
#include <memory>
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

            return std::move(proto);
        }
        else if (layout.isAudio()) // audio
        {
            // Continuous flow should only have a single region
            assert(dstRegions.size() == 1);
            return std::make_unique<IngressProtocolWriterWithBounceBuffer>(std::move(domain), layout.asAudio(), dstRegions.front());
        }
        throw Exception::invalidArgument("Unsupported data layout for ingress protocol selection.");
    }

    std::unique_ptr<EgressProtocol> selectProtocol(DataLayout const& layout)
    {
        if (layout.isVideo())
        {
            return std::make_unique<EgressProtocolWriter>(layout.asVideo());
        }
        else if (layout.isAudio()) // audio
        {
            return std::make_unique<EgressProtocolWriterWithBounceBuffer>(layout.asAudio());
        }
        throw Exception::invalidArgument("Unsupported data layout for egress protocol selection.");
    }

}
