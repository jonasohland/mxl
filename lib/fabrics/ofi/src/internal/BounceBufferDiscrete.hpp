#pragma once

#include "BounceBuffer.hpp"
#include "DataLayout.hpp"

namespace mxl::lib::fabrics::ofi
{

    class BounceBufferDiscreteUnpacker : public BounceBufferUnpacker
    {
    public:
        BounceBufferDiscreteUnpacker(DataLayout::VideoDataLayout layout)
            : _layout(std::move(layout))
        {}

        void unpack(BounceBufferEntry const&, std::uint64_t index, std::size_t count, LocalRegion& outRegion) const final;
        void unpack(BounceBufferEntry const& entry, LocalRegion& outRegion) const final;

        [[nodiscard]]
        std::size_t entrySize() const noexcept final;

    private:
        DataLayout::VideoDataLayout _layout;
    };

}
