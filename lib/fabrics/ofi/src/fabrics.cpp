#include "mxl/fabrics.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <internal/Instance.hpp>
#include <internal/Logging.hpp>
#include <mxl/mxl.h>
#include <rdma/fabric.h>
#include "internal/Exception.hpp"
#include "internal/FabricsInstance.hpp"
#include "internal/Initiator.hpp"
#include "internal/Target.hpp"
#include "internal/TargetInfo.hpp"
#include "mxl/platform.h"

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateInstance(mxlInstance in_instance, mxlFabricsInstance* out_fabricsInstance)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_instance == nullptr || out_fabricsInstance == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        *out_fabricsInstance = reinterpret_cast<mxlFabricsInstance>(new ofi::FabricsInstance(reinterpret_cast<mxl::lib::Instance*>(in_instance)));

        return MXL_STATUS_OK;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to create instance: {}", e.what());

        return e.status();
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
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_instance == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        delete reinterpret_cast<ofi::FabricsInstance*>(in_instance);

        return MXL_STATUS_OK;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to destroy instance: {}", e.what());

        return e.status();
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
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || out_target == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto instance = reinterpret_cast<ofi::FabricsInstance*>(in_fabricsInstance);

        *out_target = reinterpret_cast<mxlFabricsTarget>(instance->createTarget());
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to create target: {}", e.what());

        return e.status();
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
mxlStatus mxlFabricsDestroyTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    try
    {
        auto instance = reinterpret_cast<ofi::FabricsInstance*>(in_fabricsInstance);
        auto target = reinterpret_cast<ofi::TargetWrapper*>(in_target);

        instance->destroyTarget(target);

        return MXL_STATUS_OK;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to destroy target: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to destroy target : {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to destroy target");
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetup(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, mxlTargetConfig* in_config,
    mxlTargetInfo* out_info)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || in_target == nullptr || in_config == nullptr || out_info == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto target = reinterpret_cast<ofi::TargetWrapper*>(in_target);
        auto [status, info] = target->setup(*in_config);
        if (status == MXL_STATUS_OK && info)
        {
            *out_info = reinterpret_cast<mxlTargetInfo>(info.release());

            return MXL_STATUS_OK;
        }

        // Something went wrong, but status is ok.
        if (status == MXL_STATUS_OK)
        {
            status = MXL_ERR_UNKNOWN;
        }

        return status;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to set up target: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to set up target : {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to set up target");
        return MXL_ERR_UNKNOWN;
    }
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint64_t in_index, GrainInfo* out_grain,
    uint8_t** out_payload)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_target);
    MXL_FABRICS_UNUSED(in_index);
    MXL_FABRICS_UNUSED(out_grain);
    MXL_FABRICS_UNUSED(out_payload);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetGetGrainBlocking(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint64_t in_index,
    uint16_t in_timeoutMs, GrainInfo* out_grain, uint8_t** out_payload)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_target);
    MXL_FABRICS_UNUSED(in_index);
    MXL_FABRICS_UNUSED(in_timeoutMs);
    MXL_FABRICS_UNUSED(out_grain);
    MXL_FABRICS_UNUSED(out_payload);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetWaitForNewGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target, uint16_t in_timeoutMs,
    GrainInfo* out_grain, uint8_t** out_payload, uint64_t* out_grainIndex)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_target);
    MXL_FABRICS_UNUSED(in_timeoutMs);
    MXL_FABRICS_UNUSED(out_grain);
    MXL_FABRICS_UNUSED(out_payload);
    MXL_FABRICS_UNUSED(out_grainIndex);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetSetCompletionCallback(mxlFabricsInstance in_fabricsInstance, mxlFabricsTarget in_target,
    mxlFabricsCompletionCallback_t callbackFn)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_target);
    MXL_FABRICS_UNUSED(callbackFn);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsCreateInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator* out_initiator)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || out_initiator == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto instance = reinterpret_cast<ofi::FabricsInstance*>(in_fabricsInstance);

        *out_initiator = reinterpret_cast<mxlFabricsInitiator>(instance->createInitiator());
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to create initiator: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to create initiator : {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to create initiator");
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsDestroyInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_initiator);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorSetup(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlInitiatorConfig const* in_config)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || in_initiator == nullptr || in_config == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto initiator = reinterpret_cast<ofi::Initiator*>(in_initiator);

        return initiator->setup(*in_config);
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to set up initiator: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to set up initiator : {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to set up initiator");
        return MXL_ERR_UNKNOWN;
    }
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorAddTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || in_initiator == nullptr || in_targetInfo == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    auto initiator = reinterpret_cast<ofi::Initiator*>(in_initiator);
    auto targetInfo = ofi::TargetInfo::fromAPI(in_targetInfo);
    initiator->addTarget(*targetInfo);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorRemoveTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_fabricsInstance == nullptr || in_initiator == nullptr || in_targetInfo == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    auto initiator = reinterpret_cast<ofi::Initiator*>(in_initiator);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrain(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator, GrainInfo const* in_grainInfo,
    uint8_t const* in_payload)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_initiator);
    MXL_FABRICS_UNUSED(in_grainInfo);
    MXL_FABRICS_UNUSED(in_payload);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsInitiatorTransferGrainToTarget(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator in_initiator,
    GrainInfo const* in_grainInfo, mxlFabricsTarget const* in_targetInfo, uint8_t const* in_payload)
{
    MXL_FABRICS_UNUSED(in_fabricsInstance);
    MXL_FABRICS_UNUSED(in_initiator);
    MXL_FABRICS_UNUSED(in_grainInfo);
    MXL_FABRICS_UNUSED(in_targetInfo);
    MXL_FABRICS_UNUSED(in_payload);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsProviderFromString(char const* in_string, mxlFabricsProvider* out_provider)
{
    MXL_FABRICS_UNUSED(in_string);
    MXL_FABRICS_UNUSED(out_provider);

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsProviderToString(mxlFabricsProvider in_provider, char* out_string, size_t* in_stringSize)
{
    MXL_FABRICS_UNUSED(in_provider);
    MXL_FABRICS_UNUSED(out_string);
    MXL_FABRICS_UNUSED(in_stringSize);

    if (out_string == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    auto providerEnumValueToString = [](char* outString, size_t* inOutStringSize, char const* providerString)
    {
        if (outString == nullptr)
        {
            *inOutStringSize = ::strlen(providerString) + 1; // Null terminated.
        }
        else
        {
            if (*inOutStringSize <= ::strlen(providerString))
            {
                return MXL_ERR_STRLEN;
            }

            ::strncpy(outString, providerString, *inOutStringSize);
        }

        return MXL_STATUS_OK;
    };

    switch (in_provider)
    {
        case MXL_SHARING_PROVIDER_AUTO:  return providerEnumValueToString(out_string, in_stringSize, "auto");
        case MXL_SHARING_PROVIDER_TCP:   return providerEnumValueToString(out_string, in_stringSize, "tcp");
        case MXL_SHARING_PROVIDER_EFA:   return providerEnumValueToString(out_string, in_stringSize, "efa");
        case MXL_SHARING_PROVIDER_VERBS: return providerEnumValueToString(out_string, in_stringSize, "verbs");
        default:                         return MXL_ERR_INVALID_ARG;
    }
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetInfoFromString(char const* in_string, mxlTargetInfo* out_targetInfo)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_string == nullptr || out_targetInfo == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto targetInfo = std::make_unique<ofi::TargetInfo>();

        std::stringstream ss{in_string, std::ios_base::in};
        ss >> *targetInfo;

        *out_targetInfo = reinterpret_cast<mxlTargetInfo>(targetInfo.release());

        return MXL_STATUS_OK;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to create instance: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsTargetInfoToString(mxlTargetInfo const in_targetInfo, char* out_string, size_t* in_stringSize)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_targetInfo == nullptr || in_stringSize == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        std::stringstream ss;
        ss << *reinterpret_cast<ofi::TargetInfo const*>(in_targetInfo);

        auto targetInfoString = ss.str();

        if (out_string == nullptr)
        {
            *in_stringSize = targetInfoString.length() + 1;
        }
        else
        {
            if (*in_stringSize <= targetInfoString.length())
            {
                return MXL_ERR_STRLEN;
            }
            else
            {
                ::strncpy(out_string, targetInfoString.c_str(), *in_stringSize);
            }
        }

        return MXL_STATUS_OK;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to create instance: {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}

extern "C" MXL_EXPORT
mxlStatus mxlFabricsFreeTargetInfo(mxlTargetInfo in_info)
{
    namespace ofi = mxl::lib::fabrics::ofi;

    if (in_info == nullptr)
    {
        return MXL_ERR_INVALID_ARG;
    }

    try
    {
        auto info = reinterpret_cast<ofi::TargetInfo*>(in_info);

        delete info;
    }
    catch (ofi::Exception& e)
    {
        MXL_ERROR("Failed to destroy target info object : {}", e.what());

        return e.status();
    }
    catch (std::exception& e)
    {
        MXL_ERROR("Failed to destroy target info object: {}", e.what());
        return MXL_ERR_UNKNOWN;
    }
    catch (...)
    {
        MXL_ERROR("Failed to destroy target info object");
        return MXL_ERR_UNKNOWN;
    }

    return MXL_STATUS_OK;
}
