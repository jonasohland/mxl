#pragma once

#include "Endpoint.hpp"
#include "Protocol.hpp"
#include "RemoteRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    class EgressProtocolWriterDiscrete final : public EgressProtocol
    {
    public:
        /// Passed references should live at least as long as this instance
        EgressProtocolWriterDiscrete(Endpoint& ep, std::vector<RemoteRegion> const& remoteRegions, DataLayout::VideoDataLayout const& layout);

        std::size_t transferGrain(LocalRegion const& srcRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& srcRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint& _ep;
        std::vector<RemoteRegion> const& _remoteRegions;
        DataLayout::VideoDataLayout const& _layout;
    };

    class EgressProtocolWriterContinuous final : public EgressProtocol
    {
    public:
        /// Passed references should live at least as long as this instance
        EgressProtocolWriterContinuous(Endpoint& ep, std::vector<RemoteRegion> const& remoteRegions, DataLayout::AudioDataLayout const& layout);

        std::size_t transferGrain(LocalRegion const& srcRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& srcRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint& _ep;
        std::vector<RemoteRegion> const& _remoteRegions;
        DataLayout::AudioDataLayout const& _layout;
        size_t _bounceBufferSize;
        size_t _bounceBufferEntryIndex{0};
    };

    /// When using send/recv, a bounce buffer is always present
    class EgressProtocolSender final : public EgressProtocol
    {
    public:
        EgressProtocolSender(Endpoint& ep, DataLayout::AudioDataLayout const& layout);
        std::size_t transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex) final;
        std::size_t transferSamples(LocalRegionGroup const& localRegionGroup, std::uint64_t headIndex, std::size_t nbSamples) final;

    private:
        Endpoint& _ep;
        size_t _bounceBufferSize;
        size_t _bounceBufferEntryIndex{0};
    };

}
