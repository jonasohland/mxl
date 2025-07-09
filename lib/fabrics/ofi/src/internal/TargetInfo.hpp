#pragma once

#include <iostream>

namespace mxl::lib::fabrics::ofi
{
    class TargetInfo
    {
    public:
        TargetInfo() = default;
        friend std::ostream& operator<<(std::ostream&, TargetInfo const&);
        friend std::istream& operator>>(std::istream&, TargetInfo&);
    };

    std::ostream& operator<<(std::ostream&, TargetInfo const&);
    std::istream& operator>>(std::istream&, TargetInfo const&);
}
