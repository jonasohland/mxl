#pragma once

#include <internal/Instance.hpp>

namespace mxl::lib::fabrics
{

    class FabricsInstance
    {
    public:
        FabricsInstance(mxl::lib::Instance* instance);
        ~FabricsInstance();

        FabricsInstance(FabricsInstance&&) = delete;
        FabricsInstance(FabricsInstance const&) = delete;
        FabricsInstance& operator=(FabricsInstance&&) = delete;
        FabricsInstance& operator=(FabricsInstance const&) = delete;

    private:
        mxl::lib::Instance* _mxlInstance;
    };

}
