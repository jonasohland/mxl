#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "DataLayout.hpp"
#include "ImmData.hpp"
#include "LocalRegion.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{
    class AudioBounceBufferEntry final
    {
    public:
        AudioBounceBufferEntry(std::size_t size);
        [[nodiscard]]
        std::uint8_t const* data() const noexcept;
        [[nodiscard]]
        std::size_t size() const noexcept;
        void unpack(DataLayout::AudioDataLayout const& layout, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const noexcept;

    private:
        std::vector<std::uint8_t> _buffer;
    };

    class AudioBounceBuffer final
    {
    public:
        AudioBounceBuffer(DataLayout::AudioDataLayout const& layout);

        [[nodiscard]]
        std::vector<Region> getRegions() const noexcept;

        /// Copy the bouncing buffer entry to the LocalRegion following the internal data layout
        void unpack(std::size_t entryIndex, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const noexcept;

        /// Generate the scatter-gather list for a given head index, sample count and localRegion, using  the audio data layout
        static std::vector<LocalRegion> scatterGatherList(DataLayout::AudioDataLayout layout, std::uint64_t index, std::size_t count,
            LocalRegion const& localRegion) noexcept; // TODO: check if this makes sense here...

    private:
        friend class ImmDataSample;

    private:
        constexpr static size_t NUMBER_OF_ENTRIES = 4;

    private:
        std::vector<AudioBounceBufferEntry> _entries;
        DataLayout::AudioDataLayout _layout;
    };

}
