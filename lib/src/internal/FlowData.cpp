#include "FlowData.hpp"

namespace mxl::lib
{
    FlowData::FlowData(char const* flowFilePath, AccessMode mode)
        : _flow{flowFilePath, mode, 0U}
    {}

    FlowData::~FlowData() = default;

    mxlFlowData FlowData::toAPI() const noexcept
    {
        return reinterpret_cast<mxlFlowData>(const_cast<FlowData*>(this));
    }

    FlowData* FlowData::fromAPI(mxlFlowData flowData) noexcept
    {
        return reinterpret_cast<FlowData*>(flowData);
    }

}
