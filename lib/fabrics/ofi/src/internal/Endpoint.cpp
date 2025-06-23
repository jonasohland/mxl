#include "Endpoint.hpp"
#include <rdma/fabric.h>

namespace mxl::lib::fabrics
{
    EndpointInfo::EndpointInfo(::fi_info* info)
        : _info(info)
    {}

    EndpointInfo::~EndpointInfo()
    {
        if (_info != nullptr)
        {
            ::fi_freeinfo(_info);
            _info = nullptr;
        }
    }
}
