#include "RMATarget.hpp"
#include <cstring>
#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>
#include <rdma/fabric.h>

namespace mxl::lib::fabrics::ofi
{

    bool cmpPreferCapHMEM(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return !(lhs->caps & FI_HMEM) && (rhs->caps & FI_HMEM);
    }

    bool cmpPreferEFA(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return std::string_view{lhs->fabric_attr->prov_name} != "efa" && std::string_view{rhs->fabric_attr->prov_name} == "efa";
    }

    bool cmpPreferVerbs(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return std::string_view{lhs->fabric_attr->prov_name} != "verbs" && std::string_view{rhs->fabric_attr->prov_name} == "verbs";
    }

    bool cmpPreferSHM(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return std::string_view{lhs->fabric_attr->prov_name} != "shm" && std::string_view{rhs->fabric_attr->prov_name} == "shm";
    }

    bool cmpPreferProtoXRC(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return lhs->ep_attr->protocol != FI_PROTO_RDMA_CM_IB_XRC && rhs->ep_attr->protocol == FI_PROTO_RDMA_CM_IB_XRC;
    }

    bool cmpPreferAddrFmtSockaddr(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return lhs->addr_format != FI_SOCKADDR && rhs->addr_format == FI_SOCKADDR;
    }

    bool cmpPreferProgressAuto(FIInfoView const& lhs, FIInfoView const& rhs)
    {
        return lhs->domain_attr->progress != FI_PROGRESS_AUTO && rhs->domain_attr->progress == FI_PROGRESS_AUTO;
    }

    std::optional<FIInfoView> RMATarget::findBestFabric(FIInfoList& available, mxlFabricsProvider)
    {
        namespace ranges = std::ranges;

        std::vector<FIInfoView> candidates;

        ranges::copy_if(
            available, std::back_inserter(candidates), [](FIInfoView const& candidate) { return candidate->caps & (FI_RMA | FI_REMOTE_WRITE); });

        ranges::stable_sort(candidates, cmpPreferProgressAuto);
        ranges::stable_sort(candidates, cmpPreferCapHMEM);
        ranges::stable_sort(candidates, cmpPreferSHM);
        ranges::stable_sort(candidates, cmpPreferProtoXRC); //TODO: I think this is not required
        ranges::stable_sort(candidates, cmpPreferVerbs);
        ranges::stable_sort(candidates, cmpPreferEFA);

        if (candidates.empty())
        {
            return std::nullopt;
        }

        return candidates[0];
    }

}
