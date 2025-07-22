#include "Region.hpp"
#include <cstdint>
#include <algorithm>
#include <utility>
#include "internal/DiscreteFlowData.hpp"
#include "internal/Flow.hpp"
#include "mxl/dataformat.h"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "Exception.hpp"
#include "VariantUtils.hpp"

namespace mxl::lib::fabrics::ofi
{
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
            regions.emplace_back(Buffers{
                std::make_pair(reinterpret_cast<void*>(discreteFlow->grainInfoAt(i)), sizeof(GrainHeader)),
                std::make_pair(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(grain) + sizeof(GrainHeader)), grain->header.info.grainSize),
            });
        }

        return Regions{regions};
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

    std::ostream& operator<<(std::ostream& os, Region const&)
    {
        return os;
    }

    std::istream& operator>>(std::istream& is, Region&)
    {
        return is;
    }

    bool Regions::empty() const noexcept
    {
        return _inner.empty();
    }

    std::array<::iovec, 2> Region::to_iovec() const noexcept
    {
        return {
            ::iovec{.iov_base = buffers[0].first, .iov_len = buffers[0].second},
            ::iovec{.iov_base = buffers[1].first, .iov_len = buffers[1].second}
        };
    }

    std::ostream& operator<<(std::ostream& os, Regions const& regions)
    {
        for (auto const& region : regions._inner)
        {
            os << region;
        }
        return os;
    }

    std::istream& operator>>(std::istream& is, Regions&)
    {
        return is;
    }

    std::vector<Region> buildRegions(void**, std::size_t*, std::size_t)
    {
        throw Exception::make(MXL_ERR_UNKNOWN, "unimplemented");
    }

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
                [&](uuids::uuid const& flowId) -> Regions { return Regions::fromFlow(instance.openFlow(flowId, accessMode).get()); },
            },
            _inner);
    }
}
