#include "ProtocolEgress.hpp"
#include <cstddef>
#include "BounceBuffer.hpp"
#include "Exception.hpp"
#include "ImmData.hpp"

namespace mxl::lib::fabrics::ofi
{
    EgressProtocolWriterDiscrete::EgressProtocolWriterDiscrete(Endpoint& ep, std::vector<RemoteRegion> const& remoteRegions,
        DataLayout::VideoDataLayout const& layout)

        : _ep(ep)
        , _remoteRegions(remoteRegions)
        , _layout(layout)
    {}

    std::size_t EgressProtocolWriterDiscrete::transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex)
    {
        // Find the remote region to which this grain should be written.
        auto const& remoteRegion = _remoteRegions[grainIndex % _remoteRegions.size()];

        if (sliceIndex < _layout.totalSlices)
        {
            /// Special case for partial grain transfer with an alpha plane present
            if (_layout.alphaPlaneSize > 0)
            {
                // We are doing partial grain transfer, we need to execute 2 remote write transfers.
                // Remote Region for Color Plane
                auto region = RemoteRegion{
                    .addr = remoteRegion.addr,
                    .len = _layout.colorPlaneSize,
                    .rkey = remoteRegion.rkey,
                };
                auto nbWrites = _ep.write(localRegion.asGroup(), region, FI_ADDR_UNSPEC, ImmDataGrain{grainIndex, sliceIndex}.data());

                // Remote Region for Alpha Plane
                region = RemoteRegion{
                    .addr = remoteRegion.addr + _layout.colorPlaneSize,
                    .len = _layout.alphaPlaneSize,
                    .rkey = remoteRegion.rkey,
                };
                nbWrites += _ep.write(localRegion.asGroup(), region, FI_ADDR_UNSPEC, ImmDataGrain{grainIndex, sliceIndex}.data());
                return nbWrites;
            }
        }
        return _ep.write(localRegion.asGroup(), remoteRegion, FI_ADDR_UNSPEC, ImmDataGrain{grainIndex, sliceIndex}.data());
    }

    std::size_t EgressProtocolWriterDiscrete::transferSamples(LocalRegionGroup const&, std::uint64_t, std::size_t)
    {
        // A user probably tried to call transferSamples with an endpoint that was configured for transferring grains.
        throw Exception::internal("transferSamples for \"Writer\" protocol is not supported.");
    }

    EgressProtocolWriterContinuous::EgressProtocolWriterContinuous(Endpoint& ep, std::vector<RemoteRegion> const& remoteRegions,
        DataLayout::AudioDataLayout const& layout)
        : _ep(ep)
        , _remoteRegions(remoteRegions)
        , _layout(layout)
        , _bounceBufferSize(BounceBuffer::NUMBER_OF_ENTRIES)
    {}

    std::size_t EgressProtocolWriterContinuous::transferGrain(LocalRegion const&, std::uint64_t, std::uint16_t)
    {
        throw Exception::internal("transferGrain for \"EgressProtocolWriterContinuous\" protocol is not supported.");
    }

    std::size_t EgressProtocolWriterContinuous::transferSamples(LocalRegionGroup const& localRegionGroup, std::uint64_t partialHeadIndex,
        std::size_t sampleCount)
    {
        auto const& remote = _remoteRegions[_bounceBufferEntryIndex];

        auto nbWrites = _ep.write(
            localRegionGroup, remote, FI_ADDR_UNSPEC, ImmDataSample{_bounceBufferEntryIndex, partialHeadIndex, sampleCount}.data());

        _bounceBufferEntryIndex = (_bounceBufferEntryIndex + 1) % _bounceBufferSize;

        return nbWrites;
    }

    std::size_t EgressProtocolSender::transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex)
    {
        auto nbSends = _ep.send(localRegion.asGroup(), FI_ADDR_UNSPEC, ImmDataGrain{grainIndex, sliceIndex}.data());

        _bounceBufferEntryIndex = (_bounceBufferEntryIndex + 1) % _bounceBufferSize;

        return nbSends;
    }

    std::size_t EgressProtocolSender::transferSamples(LocalRegionGroup const& localRegionGroup, std::uint64_t partialHeadIndex,
        std::size_t sampleCount)
    {
        auto nbSends = _ep.send(localRegionGroup, FI_ADDR_UNSPEC, ImmDataSample{_bounceBufferEntryIndex, partialHeadIndex, sampleCount}.data());

        _bounceBufferEntryIndex = (_bounceBufferEntryIndex + 1) % _bounceBufferSize;

        return nbSends;
    }

}
