#include "Target.hpp"
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types

namespace mxl::lib::fabrics::ofi
{
    TargetWrapper::TargetWrapper()
    {
        MXL_INFO("Target created");
    }

    TargetWrapper::~TargetWrapper()
    {
        MXL_INFO("Target destroyed");
    }

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config)
    {
        namespace ranges = std::ranges;

        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}, flow = {}]",
            config.endpointAddress.node,
            config.endpointAddress.service,
            config.provider,
            config.flowId);

        auto infoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service);

        return {MXL_STATUS_OK, std::make_unique<TargetInfo>()};
    }
}
