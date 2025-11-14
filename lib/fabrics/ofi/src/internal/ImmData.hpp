#pragma once

#include <cstdint>
#include <utility>

namespace mxl::lib::fabrics::ofi
{

    class ImmDataDiscrete
    {
    public:
        ImmDataDiscrete(std::uint32_t data) noexcept;
        ImmDataDiscrete(std::uint64_t index, std::uint16_t sliceIndex) noexcept;

        [[nodiscard]]
        std::pair<std::uint16_t, std::uint16_t> unpack() const noexcept;
        [[nodiscard]]
        std::uint32_t data() const noexcept;

    private:
        union
        {
            struct
            {
                std::uint16_t ringBufferIndex;
                std::uint16_t sliceIndex;
            };

            uint32_t data;
        } _inner;
    };

}
