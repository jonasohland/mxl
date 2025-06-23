#include "mxl/fabrics.h"
#include <internal/Instance.hpp>
#include <internal/Logging.hpp>
#include <mxl/mxl.h>
#include <rdma/fabric.h>
#include "internal/FabricsInstance.hpp"
#include "internal/MemoryRegion.hpp"
#include "mxl/platform.h"

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateInstance(mxlInstance in_instance, mxlFabricsInstance* out_fabricsInstance)
{
    if (in_instance == nullptr || out_fabricsInstance == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        *out_fabricsInstance = reinterpret_cast<mxlFabricsInstance>(
            new mxl::lib::fabrics::FabricsInstance(reinterpret_cast<mxl::lib::Instance*>(in_instance)));

        return MXL_STATUS_OK;
    }
    catch (std::exception& e)
    {
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        return MXL_ERR_UNKNOWN;
    }
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsDestroyInstance(mxlFabricsInstance in_instance)
{
    if (in_instance == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        delete reinterpret_cast<mxl::lib::fabrics::FabricsInstance*>(in_instance);

        return MXL_STATUS_OK;
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to destroy fabrics instance: {}", e.what());

        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to destroy fabrics instance.");

        return MXL_ERR_UNKNOWN;
    }
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget* out_target)
{
    try
    {}
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to create target : {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to create target");
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsDestroyTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetup(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, mxlTargetConfig* in_config,
    mxlTargetInfo* out_info)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint64_t in_index, GrainInfo* out_grain,
    uint8_t** out_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrainBlocking(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint64_t in_index,
    uint16_t in_timeoutMs, GrainInfo* out_grain, uint8_t** out_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetWaitForNewGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint16_t in_timeoutMs,
    GrainInfo* out_grain, uint8_t** out_payload, uint64_t* out_grainIndex)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetCompletionCallback(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target,
    mxlFabricsCompletionCallback_t callbackFn)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator* out_initiator)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsDestroyInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorSetup(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlInitiatorConfig const* in_config)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorAddTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlTargetInfo const* in_targetInfo)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorRemoveTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlTargetInfo const* in_targetInfo)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, GrainInfo const* in_grainInfo,
    uint8_t const* in_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrainToTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator,
    GrainInfo const* in_grainInfo, mxlFabricsTarget const* in_targetInfo, uint8_t const* in_payload)
{
    return MXL_STATUS_OK;
}
