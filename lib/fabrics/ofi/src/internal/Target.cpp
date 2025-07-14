#include "Target.hpp"
#include <memory>
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "internal/Logging.hpp"
#include "mxl/fabrics.h"
#include "mxl/mxl.h"
#include "Domain.hpp"
#include "Exception.hpp"
#include "Fabric.hpp"
#include "FIInfo.hpp"
#include "Format.hpp" // IWYU pragma: keep; Includes template specializations of fmt::formatter for our types
#include "RMATarget.hpp"

namespace mxl::lib::fabrics::ofi
{
    TargetWrapper::TargetWrapper()
    {}

    TargetWrapper::~TargetWrapper()
    {}

    std::pair<mxlStatus, std::unique_ptr<TargetInfo>> TargetWrapper::setup(mxlTargetConfig const& config)
    {
        namespace ranges = std::ranges;

        MXL_INFO("setting up target [endpoint = {}:{}, provider = {}, flow = {}]",
            config.endpointAddress.node,
            config.endpointAddress.service,
            config.provider,
            config.flowId);

        auto fabricInfoList = FIInfoList::get(config.endpointAddress.node, config.endpointAddress.service);

        auto bestFabricInfo = RMATarget::findBestFabric(fabricInfoList, config.provider);
        if (!bestFabricInfo)
        {
            throw Exception::make(MXL_ERR_NO_FABRIC, "No suitable fabric available");
        }

        auto fabric = Fabric::open(*bestFabricInfo);
        auto domain = Domain::open(*bestFabricInfo, fabric);

        return {MXL_STATUS_OK, std::make_unique<TargetInfo>()};
    }
}
