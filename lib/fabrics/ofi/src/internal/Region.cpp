#include "Region.hpp"
#include <cstdint>
#include <algorithm>
#include <bits/types/struct_iovec.h>
#include "internal/Flow.hpp"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "Exception.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
    // BufferSpace implementations
    ::iovec BufferSpace::to_iovec() const
    {
        return ::iovec{.iov_base = reinterpret_cast<void*>(base), .iov_len = size};
    }

    std::uintptr_t Region::firstBaseAddress() const noexcept
    {
        return _inner.front().base;
    }

    // Region implementations
    std::vector<::iovec> Region::to_iovec() const noexcept
    {
        std::vector<::iovec> iovec;

        std::ranges::transform(_inner, std::back_inserter(iovec), [](BufferSpace const& iov) { return iov.to_iovec(); });
        return iovec;
    }

    // Regions implementations
    Regions Regions::fromFlow(FlowData* flow)
    {
        static_assert(sizeof(GrainHeader) == 8192);

        if (!mxlIsDiscreteDataFormat(flow->flowInfo()->common.format))
        {
            throw Exception::make(MXL_ERR_UNKNOWN, "Non-discrete flows not supported for now");
        }

        auto discreteFlow = static_cast<DiscreteFlowData*>(flow);

        std::vector<Region> regions{};

        for (std::size_t i = 0; i < discreteFlow->grainCount(); ++i)
        {
            auto grain = discreteFlow->grainAt(i);

            auto region = Region({
                BufferSpace{.base = reinterpret_cast<std::uintptr_t>(discreteFlow->grainInfoAt(i)), .size = sizeof(GrainHeader)         },
                BufferSpace{.base = reinterpret_cast<std::uintptr_t>(grain) + sizeof(GrainHeader),  .size = grain->header.info.grainSize},
            });

            regions.emplace_back(std::move(region));
        }

        return Regions{regions};
    }

    [[nodiscard]]
    Region const& Regions::at(size_t index) const noexcept
    {
        return _inner.at(index);
    }

    [[nodiscard]]
    size_t Regions::size() const
    {
        return _inner.size();
    }

    // Regions implementations
    bool Regions::empty() const noexcept
    {
        return _inner.empty();
    }

    Regions* Regions::fromAPI(mxlRegions api) noexcept
    {
        if (!api)
        {
            return nullptr;
        }

        return reinterpret_cast<Regions*>(api);
    }

    ::mxlRegions Regions::toAPI() noexcept
    {
        return reinterpret_cast<::mxlRegions>(this);
    }

    // DeferredRegions implementations

    DeferredRegions::DeferredRegions(Regions regions) noexcept
        : _inner(std::move(regions))
    {}

    DeferredRegions::DeferredRegions(uuids::uuid id) noexcept
        : _inner(id)
    {}

    Regions DeferredRegions::unwrap(Instance& instance, AccessMode accessMode)
    {
        return std::visit(
            overloaded{
                [](Regions regions) -> Regions { return regions; },
                [&](uuids::uuid const& flowId) -> Regions
                { return Regions::fromFlow(instance.openFlow(flowId, accessMode).get()); }, // TODO: check if we need to close the flow after this
            },
            _inner);
    }
}
