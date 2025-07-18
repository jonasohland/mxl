#include "TargetInfo.hpp"
#include <utility>
#include "Address.hpp"

namespace mxl::lib::fabrics::ofi
{

    TargetInfo::TargetInfo()
        : _fi_addr()
        , _regions()
        , _rkey(0)
    {}

    TargetInfo::TargetInfo(FabricAddress const& fi_addr, Regions const& regions, uint64_t rkey)
        : _fi_addr(std::move(fi_addr))
        , _regions(std::move(regions))
        , _rkey(rkey)
    {}

    FabricAddress const& TargetInfo::fabricAddress() const noexcept
    {
        return _fi_addr;
    }

    [[nodiscard]]
    Regions const& TargetInfo::regions() const noexcept
    {
        return _regions;
    }

    [[nodiscard]]
    uint64_t TargetInfo::rkey() const noexcept
    {
        return _rkey;
    }

    std::ostream& operator<<(std::ostream& os, TargetInfo const& targetInfo)
    {
        os << targetInfo._fi_addr << targetInfo._regions << targetInfo._rkey;

        return os;
    }

    std::istream& operator>>(std::istream& is, TargetInfo& targetInfo)
    {
        is >> targetInfo._fi_addr >> targetInfo._regions >> targetInfo._rkey;

        return is;
    }

}
