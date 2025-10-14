#include "BouncingBuffer.hpp"
#include <cstddef>
#include <cstdint>
#include "DataLayout.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    BouncingBufferEntry::BouncingBufferEntry(std::size_t size)
        : _entry(std::vector<std::uint8_t>(size))
    {}

    std::uint8_t const* BouncingBufferEntry::data() const noexcept
    {
        return _entry.data();
    }

    std::size_t BouncingBufferEntry::size() const noexcept
    {
        return _entry.size();
    }

    BouncingBuffer::BouncingBuffer(std::size_t nbEntries, std::size_t entrySize, DataLayout dataLayout)

        : _buffer(std::vector<BouncingBufferEntry>(nbEntries, BouncingBufferEntry{entrySize}))
        , _dataLayout(dataLayout)
    {}

    std::vector<Region> BouncingBuffer::getRegions() const noexcept
    {
        std::vector<Region> out;
        for (auto const& entry : _buffer)
        {
            out.emplace_back(reinterpret_cast<std::uintptr_t>(entry.data()), entry.size(), Region::Location::host()); // TODO: support for CUDA memory
        }
        return out;
    }
}
