#include "FabricsInstance.hpp"
#include <internal/Logging.hpp>

namespace mxl::lib::fabrics
{
    FabricsInstance::FabricsInstance(mxl::lib::Instance* instance)
        : _mxlInstance(instance)
    {
        MXL_INFO("mxl fabrics instance created");
    }

    FabricsInstance::~FabricsInstance()
    {
        MXL_INFO("mxl fabrics instance destroyed");
    }
}
