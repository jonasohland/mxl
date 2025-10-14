#pragma once

#include <cstdint>
#include <vector>
#include "DataLayout.hpp"
#include "LocalRegion.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    class BouncingBufferEntry
    {
    public:
        BouncingBufferEntry(std::size_t size);

        [[nodiscard]]
        std::uint8_t const* data() const noexcept;
        [[nodiscard]]
        std::size_t size() const noexcept;

        void unpackAudio(LocalRegion& region) const noexcept;

    private:
        std::vector<std::uint8_t> _entry;
    };

    class BouncingBuffer
    {
    public:
        BouncingBuffer(std::size_t nbEntries, std::size_t entrySize, DataLayout dataLayout);

        [[nodiscard]]
        std::vector<Region> getRegions() const noexcept;

        // Copy the bouncing buffer entry to the LocalRegion following the internal data layout
        void unpack(std::size_t entryIndex, LocalRegion& region) const;

    private:
        std::vector<BouncingBufferEntry> _buffer;
        DataLayout _dataLayout;
    };
}
