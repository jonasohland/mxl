#include "ImmData.hpp"
#include <cstdint>

namespace mxl::lib::fabrics::ofi
{
    ImmDataDiscrete::ImmDataDiscrete(std::uint32_t data) noexcept
    {
        _inner.data = data;
    }

    ImmDataDiscrete::ImmDataDiscrete(std::uint64_t index, std::uint16_t sliceIndex) noexcept
    {
        _inner.ringBufferIndex = index;
        _inner.sliceIndex = sliceIndex;
    }

    std::pair<std::uint16_t, std::uint16_t> ImmDataDiscrete::unpack() const noexcept
    {
        return {_inner.ringBufferIndex, _inner.sliceIndex};
    }

    std::uint32_t ImmDataDiscrete::data() const noexcept
    {
        return _inner.data;
    }
}
