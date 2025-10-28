#include "ProtocolEgress.hpp"
#include <cstddef>
#include "Exception.hpp"
#include "ImmData.hpp"

namespace mxl::lib::fabrics::ofi
{
    EgressProtocolWriter::EgressProtocolWriter(Endpoint ep, std::vector<RemoteRegion> remoteRegions, DataLayout::VideoDataLayout layout)

        : _ep(std::move(ep))
        , _remoteRegions(std::move(remoteRegions))
        , _layout(std::move(layout))
    {}

    std::size_t EgressProtocolWriter::transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex, std::uint16_t sliceIndex)
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

    std::size_t EgressProtocolWriter::transferSamples(LocalRegionGroup const&, std::uint64_t, std::size_t)
    {
        // A user probably tried to call transferSamples with an endpoint that was configured for transferring grains.
        throw Exception::internal("transferSamples for \"Writer\" protocol is not supported.");
    }

    EgressProtocolWriterWithBounceBuffer::EgressProtocolWriterWithBounceBuffer(Endpoint ep, std::vector<RemoteRegion> remoteRegions,
        DataLayout::AudioDataLayout layout)
        : _ep(std::move(ep))
        , _remoteRegions(std::move(remoteRegions))
        , _layout(std::move(layout))
        ,
    {}

    std::size_t EgressProtocolWriterWithBounceBuffer::transferGrain(LocalRegion const& localRegion, std::uint64_t grainIndex,
        std::uint16_t sliceIndex)
    {
        // Exactly the same as without a bounce buffer, we can recover the bounce buffer entry by using the grain index
        // TODO: shared code with EgressProtocolWriter
        auto const& remote = _remoteRegions[grainIndex % _remoteRegions.size()];

        return _ep.write(localRegion.asGroup(), remote, FI_ADDR_UNSPEC, ImmDataGrain{grainIndex, sliceIndex}.data());
    }

    std::size_t EgressProtocolWriterWithBounceBuffer::transferSamples(LocalRegionGroup const& localRegionGroup, std::uint64_t partialHeadIndex,
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
