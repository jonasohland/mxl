#pragma once

#include <cstdint>
#include "LocalRegion.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    class BounceBufferEntry
    {
    public:
        BounceBufferEntry(std::size_t size);

        [[nodiscard]]
        std::uint8_t const* data() const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

    private:
        std::vector<std::uint8_t> _buffer;
    };

    class BounceBufferUnpacker
    {
    public:
        virtual void unpack(BounceBufferEntry const&, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const = 0;
        virtual void unpack(BounceBufferEntry const&, LocalRegion& outRegion) const = 0;

        [[nodiscard]]
        virtual std::size_t entrySize() const noexcept = 0;
    };

    class BounceBuffer
    {
    public:
        constexpr static size_t NUMBER_OF_ENTRIES = 4;

    public:
        BounceBuffer(std::unique_ptr<BounceBufferUnpacker> unpacker);

        [[nodiscard]]
        std::vector<Region> getRegions() const noexcept;

        void unpack(std::size_t entryIndex, LocalRegion& outRegion) const noexcept;
        void unpack(std::size_t entryIndex, LocalRegion& outRegion, std::uint64_t headIndex, std::size_t nbItems) const noexcept;

    private:
        friend class ImmDataSample;

    private:
        std::unique_ptr<BounceBufferUnpacker> _unpacker;
        std::vector<BounceBufferEntry> _entries;
    };
}
