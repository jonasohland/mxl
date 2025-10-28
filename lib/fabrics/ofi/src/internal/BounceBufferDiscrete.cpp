#include "BounceBufferDiscrete.hpp"
#include <cassert>
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    void BounceBufferDiscreteUnpacker::unpack(BounceBufferEntry const&, std::uint64_t, std::size_t, LocalRegion&) const
    {
        throw Exception::internal("Attempted to unpack continuous data layout using a discrete data layout Unpacker.");
    }

    void BounceBufferDiscreteUnpacker::unpack(BounceBufferEntry const& entry, LocalRegion& outRegion) const
    {
        // Validate that the BounceBuffer entry isof same size as the region we are copying to
        assert(outRegion.len == entry.size());
        // Each entry contains a full grain (color + optional alpha)
        auto srcAddr = entry.data();
        std::memcpy(reinterpret_cast<void*>(outRegion.addr), srcAddr, outRegion.len);
    }

    std::size_t BounceBufferDiscreteUnpacker::entrySize() const noexcept
    {
        return _layout.colorPlaneSize + _layout.alphaPlaneSize;
    }

}
