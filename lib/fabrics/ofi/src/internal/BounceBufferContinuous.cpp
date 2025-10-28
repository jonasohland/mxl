#include "BounceBufferContinuous.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "internal/ContinuousFlowData.hpp"
#include "DataLayout.hpp"
#include "Exception.hpp"
#include "LocalRegion.hpp"

namespace mxl::lib::fabrics::ofi
{

    void BounceBufferContinuousUnpacker::unpack(BounceBufferEntry const&, LocalRegion&) const
    {
        throw Exception::internal("Attempted to unpack discrete data layout using a continous data layout Unpacker.");
    }

    void BounceBufferContinuousUnpacker::unpack(BounceBufferEntry const& entry, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const
    {
        /// Using the given audio data layout, head index and number of samples recover the slices
        mxlMutableWrappedMultiBufferSlice slices;
        getMultiBufferSlices(index,
            count,
            _layout.samplesPerChannel,
            _layout.bytesPerSample,
            _layout.channelCount,
            reinterpret_cast<std::uint8_t*>(outRegion.addr),
            slices);

        auto srcAddr = entry.data();
        for (auto& fragment : slices.base.fragments)
        {
            // check if the fragment present
            if (fragment.size > 0)
            {
                for (size_t chan = 0; chan < slices.count; chan++)
                {
                    auto dstAddr = reinterpret_cast<std::uintptr_t>(fragment.pointer) + (slices.stride * chan);
                    std::memcpy(reinterpret_cast<void*>(dstAddr), srcAddr, fragment.size);
                    srcAddr += fragment.size;
                }
            }
        }
    }

    std::size_t BounceBufferContinuousUnpacker::entrySize() const noexcept
    {
        // Calculate the size of a bounce buffer entry needed to hold the given number of samples for all channels
        return _layout.channelCount * _layout.samplesPerChannel * _layout.bytesPerSample;
    }

    std::vector<LocalRegion> scatterGatherList(DataLayout::AudioDataLayout layout, std::uint64_t headIndex, std::size_t nbSamples,
        LocalRegion const& localRegion) noexcept
    {
        mxlWrappedMultiBufferSlice slice = {};
        getMultiBufferSlices(headIndex,
            nbSamples,
            layout.samplesPerChannel,
            layout.bytesPerSample,
            layout.channelCount,
            reinterpret_cast<std::uint8_t const*>(localRegion.addr),
            slice);

        // Create the scatter-gather list using the slices. We create at least one scatter-gather entry per channel. We potentially create an
        // additional one per channel if 2 fragments are present (wrap-around). When a fragment is not present its size will be 0.
        std::vector<LocalRegion> sgList;
        for (auto& fragment : slice.base.fragments)
        {
            // check if the fragment present
            if (fragment.size > 0)
            {
                for (size_t chan = 0; chan < slice.count; chan++)
                {
                    auto srcAddr = reinterpret_cast<std::uintptr_t>(fragment.pointer) + (slice.stride * chan);

                    sgList.emplace_back(LocalRegion{.addr = srcAddr, .len = fragment.size, .desc = localRegion.desc});
                }
            }
        }

        return sgList;
    }

}
