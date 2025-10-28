#include "ImmData.hpp"
#include <cstddef>
#include <cstdint>
#include "BounceBuffer.hpp"

namespace mxl::lib::fabrics::ofi
{
    LocalRegion ImmediateDataLocation::toLocalRegion() noexcept
    {
        auto addr = &data;

        return LocalRegion{
            .addr = reinterpret_cast<uint64_t>(reinterpret_cast<std::uintptr_t>(addr)),
            .len = sizeof(uint64_t),
            .desc = nullptr,
        };
    }

    ImmDataGrain::ImmDataGrain(std::uint32_t data) noexcept
    {
        _inner.data = data;
    }

    ImmDataGrain::ImmDataGrain(std::uint64_t grainIndex, std::uint16_t sliceIndex) noexcept
    {
        _inner._repr.grainIndex = grainIndex;
        _inner._repr.sliceIndex = sliceIndex;
    }

    std::pair<std::uint64_t, std::uint16_t> ImmDataGrain::unpack() const noexcept
    {
        return {_inner._repr.grainIndex, _inner._repr.sliceIndex};
    }

    std::uint32_t ImmDataGrain::data() const noexcept
    {
        return _inner.data;
    }

    ImmDataSample::ImmDataSample(std::uint32_t data) noexcept
    {
        _inner.data = data;
    }

    ImmDataSample::ImmDataSample(std::uint64_t entryIndex, std::uint64_t headIndex, std::size_t count) noexcept
    {
        static_assert(BounceBuffer::NUMBER_OF_ENTRIES == 4,
            "This code assumes that the maximum number of entries in the bounce buffer is 4. This code needs to be update if that changes");
        _inner._repr.entryIndex = entryIndex;
        _inner._repr.headIndex = headIndex & 0x7FFF;
        _inner._repr.count = count & 0x7FFF;
    }

    std::tuple<std::size_t, std::uint16_t, std::size_t> ImmDataSample::unpack() const noexcept
    {
        static_assert(BounceBuffer::NUMBER_OF_ENTRIES == 4,
            "This code assumes that the maximum number of entries in the bounce buffer is 4. This code needs to be update if that changes");
        return {_inner._repr.entryIndex, _inner._repr.headIndex, _inner._repr.count};
    }

    std::uint32_t ImmDataSample::data() const noexcept
    {
        return _inner.data;
    }

}
