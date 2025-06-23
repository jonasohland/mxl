#include "mxl/fabrics.h"
#include <Instance.hpp>
#include <Logging.hpp>
#include <mxl/mxl.h>
#include "internal/MemoryRegion.hpp"
#include "mxl/platform.h"

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTest()
{
    mxlFabricsTestInternal();

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateTarget(mxlInstance in_instance, mxlFabricsTarget* out_target)
{
    try
    {
        auto instance = reinterpret_cast<mxl::lib::Instance*>(in_instance);
    }
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
mxlStatus mxlFabricsDestroyTarget(mxlInstance in_instance, mxlFabricsTarget in_target)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetup(mxlInstance in_instance, mxlFabricsTarget in_target, mxlTargetConfig* in_config, mxlTargetInfo* out_info)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrain(mxlInstance in_instance, mxlFabricsTarget in_target, uint64_t in_index, GrainInfo* out_grain,
    uint8_t** out_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrainBlocking(mxlInstance in_instance, mxlFabricsTarget in_target, uint64_t in_index, uint16_t in_timeoutMs,
    GrainInfo* out_grain, uint8_t** out_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetWaitForNewGrain(mxlInstance in_instance, mxlFabricsTarget in_target, uint16_t in_timeoutMs, GrainInfo* out_grain,
    uint8_t** out_payload, uint64_t* out_grainIndex)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetCompletionCallback(mxlInstance in_instance, mxlFabricsTarget in_target, mxlFabricsCompletionCallback_t callbackFn)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateInitiator(mxlInstance in_instance, mxlFabricsInitiator* out_initiator)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsDestroyInitiator(mxlInstance in_instance, mxlFabricsInitiator in_initiator)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorSetup(mxlInstance in_instance, mxlFabricsInitiator in_initiator, mxlInitiatorConfig const* in_config)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorAddTarget(mxlInstance in_instance, mxlFabricsInitiator in_initiator, mxlTargetInfo const* in_targetInfo)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorRemoveTarget(mxlInstance in_instance, mxlFabricsInitiator in_initiator, mxlTargetInfo const* in_targetInfo)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrain(mxlInstance in_instance, mxlFabricsInitiator in_initiator, GrainInfo const* in_grainInfo,
    uint8_t const* in_payload)
{
    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrainToTarget(mxlInstance in_instance, mxlFabricsInitiator in_initiator, GrainInfo const* in_grainInfo,
    mxlFabricsTarget const* in_targetInfo, uint8_t const* in_payload)
{
    return MXL_STATUS_OK;
}
