#include "AudioBounceBuffer.hpp"
#include <cstddef>
#include <vector>
#include "internal/ContinuousFlowData.hpp"
#include "DataLayout.hpp"
#include "LocalRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    AudioBounceBufferEntry::AudioBounceBufferEntry(std::size_t size)
    {
        _buffer = std::vector<std::uint8_t>(size);
    }

    std::uint8_t const* AudioBounceBufferEntry::data() const noexcept
    {
        return _buffer.data();
    }

    std::size_t AudioBounceBufferEntry::size() const noexcept
    {
        return _buffer.size();
    }

    void AudioBounceBufferEntry::unpack(DataLayout::AudioDataLayout const& layout, std::uint64_t index, std::size_t sampleCount,
        LocalRegion& region) const noexcept
    {
        /// Using the given audio data layout, head index and number of samples recover the slices
        mxlMutableWrappedMultiBufferSlice slices;
        getMultiBufferSlices(index,
            sampleCount,
            layout.samplesPerChannel,
            layout.bytesPerSample,
            layout.channelCount,
            reinterpret_cast<std::uint8_t*>(region.addr),
            slices);

        std::uintptr_t srcAddr = 0;
        for (auto& fragment : slices.base.fragments)
        {
            // check if the fragment present
            if (fragment.size > 0)
            {
                for (size_t chan = 0; chan < slices.count; chan++)
                {
                    auto dstAddr = reinterpret_cast<std::uintptr_t>(fragment.pointer) + (slices.stride * chan);
                    std::memcpy(reinterpret_cast<void*>(dstAddr), reinterpret_cast<void*>(srcAddr), fragment.size);
                    srcAddr += fragment.size;
                }
            }
        }
    }

    AudioBounceBuffer::AudioBounceBuffer(DataLayout::AudioDataLayout const& layout)

    {
        _entries = std::vector<AudioBounceBufferEntry>(NUMBER_OF_ENTRIES,
            AudioBounceBufferEntry{layout.channelCount * layout.samplesPerChannel * layout.bytesPerSample});
        _layout = layout;
    }

    std::vector<Region> AudioBounceBuffer::getRegions() const noexcept
    {
        std::vector<Region> out;
        for (auto const& entry : _entries)
        {
            out.emplace_back(reinterpret_cast<std::uintptr_t>(entry.data()), entry.size(), Region::Location::host());
        }
        return out;
    }

    void AudioBounceBuffer::unpack(std::size_t entryIndex, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const noexcept
    {
        _entries.at(entryIndex).unpack(_layout, index, count, outRegion);
    }

    std::vector<LocalRegion> AudioBounceBuffer::scatterGatherList(DataLayout::AudioDataLayout layout, std::uint64_t index, std::size_t count,
        LocalRegion const& localRegion) noexcept
    {
        mxlWrappedMultiBufferSlice slice = {};
        getMultiBufferSlices(index,
            count,
            layout.samplesPerChannel,
            layout.bytesPerSample,
            layout.bytesPerSample,
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
                    auto chanAddr = reinterpret_cast<std::uintptr_t>(fragment.pointer) + (slice.stride * chan);
                    sgList.emplace_back(LocalRegion{.addr = chanAddr, .len = fragment.size, .desc = localRegion.desc});
                }
            }
        }

        return sgList;
    }

}
