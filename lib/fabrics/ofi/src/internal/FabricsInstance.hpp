#pragma once

#include <list>
#include <internal/Instance.hpp>
#include <mxl/fabrics.h>
#include "Target.hpp"

namespace mxl::lib::fabrics::ofi
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

        TargetWrapper* createTarget();
        void destroyTarget(TargetWrapper*);

    private:
        mxl::lib::Instance* _mxlInstance;
        std::list<TargetWrapper> _targets;
    };

}
