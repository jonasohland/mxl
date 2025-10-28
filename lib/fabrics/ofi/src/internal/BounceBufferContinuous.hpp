#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include "BounceBuffer.hpp"
#include "DataLayout.hpp"
#include "LocalRegion.hpp"
#include "Region.hpp"

namespace mxl::lib::fabrics::ofi
{

    class BounceBufferContinuousUnpacker : public BounceBufferUnpacker
    {
    public:
        BounceBufferContinuousUnpacker(DataLayout::AudioDataLayout layout)
            : _layout(std::move(layout))
        {}

        void unpack(BounceBufferEntry const&, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const final;
        void unpack(BounceBufferEntry const& entry, LocalRegion& outRegion) const final;

        [[nodiscard]]
        std::size_t entrySize() const noexcept final;

    private:
        DataLayout::AudioDataLayout _layout;
    };

    std::vector<LocalRegion> scatterGatherList(DataLayout::AudioDataLayout layout, std::uint64_t headIndex, std::size_t nbSamples,
        LocalRegion const& localRegion) noexcept;

}
