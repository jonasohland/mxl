#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>
#include "LocalRegion.hpp"

namespace mxl::lib::fabrics::ofi
{
    struct ImmediateDataLocation
    {
    public:
        LocalRegion toLocalRegion() noexcept;

    public:
        std::uint64_t data;
    };

    class ImmDataGrain
    {
    public:
        ImmDataGrain(std::uint32_t data) noexcept;
        ImmDataGrain(std::uint64_t grainIndex, std::uint16_t sliceIndex) noexcept;

        [[nodiscard]]
        std::pair<std::uint64_t, std::uint16_t> unpack() const noexcept;
        [[nodiscard]]
        std::uint32_t data() const noexcept;

    private:
        union
        {
            struct
            {
                std::uint16_t grainIndex;
                std::uint16_t sliceIndex;
            } _repr;

            uint32_t data;
        } _inner;
    };

    class ImmDataSample
    {
    public:
        ImmDataSample(std::uint32_t data) noexcept;
        ImmDataSample(std::size_t entryIndex, std::uint64_t headIndex, std::size_t count) noexcept;

        [[nodiscard]]
        std::tuple<std::size_t, std::uint16_t, std::size_t> unpack() const noexcept;
        [[nodiscard]]
        std::uint32_t data() const noexcept;

    private:
        union
        {
            struct
            {
                std::uint32_t entryIndex : 2;  // enough for 4 out-standing entries
                std::uint32_t headIndex  : 15; // Sample index associated with the transfer (enough for ~340ms @ 96KHz)
                std::uint32_t count      : 15; // Enough for ~340ms @ 96KHz
            } _repr;

            uint32_t data;
        } _inner;
    };

}
