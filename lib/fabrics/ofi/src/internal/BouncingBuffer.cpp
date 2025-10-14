#include "BouncingBuffer.hpp"
#include <cstddef>
#include <cstdint>
#include "DataLayout.hpp"
#include "Exception.hpp"
#include "LocalRegion.hpp"
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

    void BouncingBufferEntry::unpackAudio(LocalRegion& region) const noexcept
    {
        // TODO:
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

    void BouncingBuffer::unpack(std::size_t entryIndex, LocalRegion& region) const
    {
        if (_dataLayout.isAudio())
        {
            _buffer.at(entryIndex).unpackAudio(region);
        }
        else if (_dataLayout.isVideo())
        {
            throw Exception::internal("Unpacking bouncing buffer for video datalayout is unsupported yet.");
        }
    }
}
