#include "BounceBuffer.hpp"

namespace mxl::lib::fabrics::ofi
{
    BounceBufferEntry::BounceBufferEntry(std::size_t size)
    {
        _buffer = std::vector<std::uint8_t>(size);
    }

    std::uint8_t const* BounceBufferEntry::data() const noexcept
    {
        return _buffer.data();
    }

    std::size_t BounceBufferEntry::size() const noexcept
    {
        return _buffer.size();
    }

    BounceBuffer::BounceBuffer(std::unique_ptr<BounceBufferUnpacker> unpacker)
        : _unpacker(std::move(unpacker))
        , _entries(std::vector<BounceBufferEntry>(NUMBER_OF_ENTRIES, BounceBufferEntry{_unpacker->entrySize()}))

    {}

    std::vector<Region> BounceBuffer::getRegions() const noexcept
    {
        std::vector<Region> out;
        for (auto const& entry : _entries)
        {
            out.emplace_back(reinterpret_cast<std::uintptr_t>(entry.data()), entry.size(), Region::Location::host());
        }
        return out;
    }

    void BounceBuffer::unpack(std::size_t entryIndex, LocalRegion& outRegion) const noexcept
    {
        _unpacker->unpack(_entries.at(entryIndex), outRegion);
    }

    void BounceBuffer::unpack(std::size_t entryIndex, LocalRegion& outRegion, std::uint64_t index, std::size_t count) const noexcept
    {
        _unpacker->unpack(_entries.at(entryIndex), index, count, outRegion);
    }

}
