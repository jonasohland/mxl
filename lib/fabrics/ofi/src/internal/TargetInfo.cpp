#include "TargetInfo.hpp"
#include "Exception.hpp"

namespace mxl::lib::fabrics::ofi
{

    std::ostream& operator<<(std::ostream& os, TargetInfo const&)
    {
        os << "{}";

        return os;
    }

    std::istream& operator>>(std::istream& is, TargetInfo&)
    {
        if (is.get() != '{')
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "invalid target info format");
        }

        if (is.get() != '}')
        {
            throw Exception::make(MXL_ERR_INVALID_ARG, "invalid target info format");
        }

        return is;
    }

}
