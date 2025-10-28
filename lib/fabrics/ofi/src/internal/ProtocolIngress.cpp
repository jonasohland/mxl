#include "ProtocolIngress.hpp"
#include "BounceBufferContinuous.hpp"
#include "BounceBufferDiscrete.hpp"
#include "Domain.hpp"
#include "ImmData.hpp"

namespace mxl::lib::fabrics::ofi
{
    void IngressProtocolWriter::processCompletion(std::uint32_t)
    {
        // No action needed for writer target
    }

    IngressProtocolWriterWithBounceBuffer::IngressProtocolWriterWithBounceBuffer(std::shared_ptr<Domain> domain, DataLayout::AudioDataLayout layout,
        Region dstRegion)
        : _domain(std::move(domain))
        , _bounceBuffer(BounceBuffer(std::make_unique<BounceBufferContinuousUnpacker>(std::move(layout))))
        , _dstRegion(dstRegion.toLocal())
    {
        _domain->registerRegions(_bounceBuffer.getRegions(), FI_REMOTE_WRITE);
    }

    void IngressProtocolWriterWithBounceBuffer::processCompletion(std::uint32_t immData)
    {
        auto [entryIndex, ringBufferHeadIndex, count] = ImmDataSample{immData}.unpack();

        _bounceBuffer.unpack(entryIndex, _dstRegion, ringBufferHeadIndex, count);
    }

    IngressProtocolReceiverDiscrete::IngressProtocolReceiverDiscrete(DataLayout::VideoDataLayout layout,
        std::shared_ptr<std::vector<LocalRegion>> localRegions)
        : _bounceBuffer(BounceBuffer(std::make_unique<BounceBufferDiscreteUnpacker>(std::move(layout))))
        , _localRegions(std::move(localRegions))
    {}

    void IngressProtocolReceiverDiscrete::processCompletion(std::uint32_t immData)
    {
        auto [grainIndex, sliceIndex] = ImmDataGrain{immData}.unpack();

        auto localRegion = _localRegions->at(grainIndex % _localRegions->size());

        auto entryIndex = grainIndex % BounceBuffer::NUMBER_OF_ENTRIES;
        _bounceBuffer.unpack(entryIndex, localRegion);
    }

    IngressProtocolReceiverContinuous::IngressProtocolReceiverContinuous(DataLayout::AudioDataLayout layout, std::shared_ptr<LocalRegion> localRegion)
        : _bounceBuffer(BounceBuffer(std::make_unique<BounceBufferContinuousUnpacker>(std::move(layout))))
        , _localRegion(std::move(localRegion))
    {}

    void IngressProtocolReceiverContinuous::processCompletion(std::uint32_t immData)
    {
        auto [entryIndex, ringBufferHeadIndex, count] = ImmDataSample{immData}.unpack();

        _bounceBuffer.unpack(entryIndex, *_localRegion, ringBufferHeadIndex, count);
    }

}
