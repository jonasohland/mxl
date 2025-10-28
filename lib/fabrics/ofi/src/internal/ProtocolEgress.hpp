#pragma once

#include "Endpoint.hpp"
#include "Protocol.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    class EgressProtocolWriter : public EgressProtocol
    {
    public:
        EgressProtocolWriter(Endpoint ep, std::vector<RemoteRegion> remoteRegions, DataLayout::VideoDataLayout layout);

        std::size_t transferGrain(LocalRegion const& srcRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& srcRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint _ep;
        std::vector<RemoteRegion> _remoteRegions;
        DataLayout::VideoDataLayout _layout;
    };

    class EgressProtocolWriterWithBounceBuffer : public EgressProtocol
    {
    public:
        EgressProtocolWriterWithBounceBuffer(Endpoint ep, std::vector<RemoteRegion> remoteRegions, DataLayout::AudioDataLayout layout);

        std::size_t transferGrain(LocalRegion const& srcRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& srcRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint _ep;
        std::vector<RemoteRegion> _remoteRegions;
        size_t _bounceBufferSize{0};
        size_t _bounceBufferEntryIndex{0};
    };

    /// When using send/recv, a bounce buffer is always present
    class EgressProtocolSender : public EgressProtocol
    {
    public:
        std::size_t transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& localRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint _ep;
        size_t _bounceBufferSize{0};
        size_t _bounceBufferEntryIndex{0};
    };

}
