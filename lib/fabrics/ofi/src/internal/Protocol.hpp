#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include "DataLayout.hpp"
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "LocalRegion.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class IngressProtocol

    {
    public:
        virtual void processCompletion(std::uint32_t immData) = 0;
    };

    class EgressProtocol
    {
    public:
        virtual std::size_t transferGrain(LocalRegion const& srcRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) = 0;
        virtual std::size_t transferSamples(LocalRegionGroup const& srcRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) = 0;
    };

    std::unique_ptr<IngressProtocol> selectProtocol(std::shared_ptr<Domain> domain, DataLayout const& layout, std::vector<Region> const& regions);
    std::unique_ptr<EgressProtocol> selectProtocol(Endpoint& ep, DataLayout const& layout, TargetInfo const& targetInfo);

}
