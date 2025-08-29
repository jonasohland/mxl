// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "mxl/fabrics.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <rfl.hpp>
#include <internal/Instance.hpp>
#include <internal/Logging.hpp>
#include <mxl/mxl.h>
#include <rdma/fabric.h>
#include <rfl/json.hpp>
#include "internal/Exception.hpp"
#include "internal/FabricsInstance.hpp"
#include "internal/FlowReader.hpp"
#include "internal/Initiator.hpp"
#include "internal/Region.hpp"
#include "internal/Target.hpp"
#include "internal/TargetInfo.hpp"
#include "mxl/flow.h"
#include "mxl/platform.h"

namespace
{
    namespace rdma = mxl::lib::fabrics::rdma_core;
    namespace mxl = mxl::lib;

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsRegionsForFlowReader(mxlFlowReader in_reader, mxlRegions* out_regions)
    {
        if (in_reader == nullptr || out_regions == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto reader = ::mxl::lib::to_FlowReader(in_reader);

            // We are leaking the ownership, the user is responsible for calling mxlFabricsRegionsFree to free the memory.
            auto regionPtr = std::make_unique<rdma::RegionGroups>(rdma::RegionGroups::fromFlow(reader->getFlowData())).release();

            *out_regions = regionPtr->toAPI();

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }
        catch (std::exception& e)
        {
            MXL_ERROR("Failed to create Regions object: {}", e.what());

            return MXL_ERR_UNKNOWN;
        }

        catch (...)
        {
            MXL_ERROR("Failed to create Regions object.");

            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsRegionsForFlowWriter(mxlFlowWriter in_writer, mxlRegions* out_regions)
    {
        if (in_writer == nullptr || out_regions == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto reader = ::mxl::lib::to_FlowWriter(in_writer);

            // We are leaking the ownership, the user is responsible for calling mxlFabricsRegionsFree to free the memory.
            auto regionPtr = std::make_unique<rdma::RegionGroups>(rdma::RegionGroups::fromFlow(reader->getFlowData())).release();

            *out_regions = regionPtr->toAPI();

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to create Regions object: {}", e.what());

            return MXL_ERR_UNKNOWN;
        }

        catch (...)
        {
            MXL_ERROR("Failed to create Regions object.");

            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsRegionsFromBufferGroups(mxlFabricsMemoryRegionGroup const* in_groups, size_t in_count, mxlRegions* out_regions)
    {
        if (in_groups == nullptr || out_regions == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto regions = rdma::RegionGroups::fromGroups(in_groups, in_count);

            // We are leaking the ownership, the user is responsible for calling mxlFabricsRegionsFree to free the memory.
            auto regionPtr = std::make_unique<rdma::RegionGroups>(regions).release();

            *out_regions = regionPtr->toAPI();

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to create Regions object: {}", e.what());

            return MXL_ERR_UNKNOWN;
        }

        catch (...)
        {
            MXL_ERROR("Failed to create Regions object.");

            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsRegionsFree(mxlRegions in_regions)
    {
        if (in_regions == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto regions = rdma::RegionGroups::fromAPI(in_regions);
            delete regions;

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to free Regions object: {}", e.what());

            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to free Regions object.");

            return MXL_ERR_UNKNOWN;
        }
    }

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
                new rdma::FabricsInstance(reinterpret_cast<::mxl::lib::Instance*>(in_instance)));

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
        if (in_instance == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            delete rdma::FabricsInstance::fromAPI(in_instance);

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
        if (in_fabricsInstance == nullptr || out_target == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto instance = rdma::FabricsInstance::fromAPI(in_fabricsInstance);
            *out_target = instance->createTarget()->toAPI();
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
        try
        {
            auto instance = rdma::FabricsInstance::fromAPI(in_fabricsInstance);
            auto target = rdma::TargetWrapper::fromAPI(in_target);

            instance->destroyTarget(target);

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
    mxlStatus mxlFabricsTargetSetup(mxlFabricsTarget in_target, mxlTargetConfig* in_config, mxlTargetInfo* out_info)
    {
        if (in_target == nullptr || in_config == nullptr || out_info == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            // Set up the target, release the returned unique_ptr, convert to external API type, assign the the pointer location
            // passed by the user.

            *out_info = rdma::TargetWrapper::fromAPI(in_target)->setup(*in_config).release()->toAPI();

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
    mxlStatus mxlFabricsTargetTryNewGrain(mxlFabricsTarget in_target, uint64_t* out_index)
    {
        if (in_target == nullptr || out_index == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto target = rdma::TargetWrapper::fromAPI(in_target);
            auto res = target->read();
            if (res.grainAvailable.has_value())
            {
                *out_index = res.grainAvailable.value();
                return MXL_STATUS_OK;
            }

            return MXL_ERR_NOT_READY;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to try for new grain : {}", e.what());
            return MXL_ERR_UNKNOWN;
        }

        catch (...)
        {
            MXL_ERROR("Failed to try for new grain");
            return MXL_ERR_UNKNOWN;
        }

        return MXL_STATUS_OK;
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsTargetWaitForNewGrain(mxlFabricsTarget in_target, uint64_t* out_index, uint16_t in_timeoutMs)
    {
        if (in_target == nullptr || out_index == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto target = rdma::TargetWrapper::fromAPI(in_target);
            auto res = target->readBlocking(std::chrono::milliseconds(in_timeoutMs));
            if (res.grainAvailable)
            {
                *out_index = res.grainAvailable.value();
                return MXL_STATUS_OK;
            }

            return MXL_ERR_TIMEOUT;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to try for new grain : {}", e.what());
            return MXL_ERR_UNKNOWN;
        }

        catch (...)
        {
            MXL_ERROR("Failed to try for new grain");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsCreateInitiator(mxlFabricsInstance in_fabricsInstance, mxlFabricsInitiator* out_initiator)
    {
        if (in_fabricsInstance == nullptr || out_initiator == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto instance = rdma::FabricsInstance::fromAPI(in_fabricsInstance);

            *out_initiator = instance->createInitiator()->toAPI();
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
        if (in_fabricsInstance == nullptr || in_initiator == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto instance = rdma::FabricsInstance::fromAPI(in_fabricsInstance);
            instance->destroyInitiator(rdma::InitiatorWrapper::fromAPI(in_initiator));
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
    mxlStatus mxlFabricsInitiatorSetup(mxlFabricsInitiator in_initiator, mxlInitiatorConfig const* in_config)
    {
        if (in_initiator == nullptr || in_config == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            rdma::InitiatorWrapper::fromAPI(in_initiator)->setup(*in_config);
            MXL_INFO("Done setup");

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
    mxlStatus mxlFabricsInitiatorAddTarget(mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo)
    {
        if (in_initiator == nullptr || in_targetInfo == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto initiator = rdma::InitiatorWrapper::fromAPI(in_initiator);
            auto targetInfo = rdma::TargetInfo::fromAPI(in_targetInfo);
            initiator->addTarget(*targetInfo);

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to add target to initiator: {}", e.what());
            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to add target to initiator");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsInitiatorRemoveTarget(mxlFabricsInitiator in_initiator, mxlTargetInfo const in_targetInfo)
    {
        if (in_initiator == nullptr || in_targetInfo == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }
        try
        {
            auto initiator = rdma::InitiatorWrapper::fromAPI(in_initiator);
            auto targetInfo = rdma::TargetInfo::fromAPI(in_targetInfo);

            initiator->removeTarget(*targetInfo);

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to remove target from initiator : {}", e.what());
            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to remove target from initiator");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsInitiatorTransferGrain(mxlFabricsInitiator in_initiator, uint64_t in_grainIndex)
    {
        if (in_initiator == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            rdma::InitiatorWrapper::fromAPI(in_initiator)->transferGrain(in_grainIndex);

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to transfer grain: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to transfer grain: {}", e.what());
            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to transfer grain");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsInitiatorMakeProgressNonBlocking(mxlFabricsInitiator in_initiator)
    {
        if (in_initiator == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            if (rdma::InitiatorWrapper::fromAPI(in_initiator)->makeProgress())
            {
                return MXL_ERR_NOT_READY;
            }

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to make progress : {}", e.what());
            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to make progress in the initiator");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsInitiatorMakeProgressBlocking(mxlFabricsInitiator in_initiator, uint16_t in_timeoutMs)
    {
        if (in_initiator == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            if (rdma::InitiatorWrapper::fromAPI(in_initiator)->makeProgressBlocking(std::chrono::milliseconds(in_timeoutMs)))
            {
                return MXL_ERR_NOT_READY;
            }

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

            return e.status();
        }

        catch (std::exception& e)
        {
            MXL_ERROR("Failed to make progress : {}", e.what());
            return MXL_ERR_UNKNOWN;
        }
        catch (...)
        {
            MXL_ERROR("Failed to make progress in the initiator");
            return MXL_ERR_UNKNOWN;
        }
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsProviderFromString(char const*, mxlFabricsProvider* out_provider)
    {
        *out_provider = MXL_SHARING_PROVIDER_AUTO;
        MXL_WARN("Provider not supported for this implementation. Always using RDMA");

        return MXL_STATUS_OK;
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsProviderToString(mxlFabricsProvider, char*, size_t*)
    {
        MXL_WARN("Provider not supported for this implementation. Always using RDMA");

        return MXL_STATUS_OK;
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsTargetInfoFromString(char const* in_string, mxlTargetInfo* out_targetInfo)
    {
        if (in_string == nullptr || out_targetInfo == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto deserializedTargetInfo = rfl::json::read<rdma::TargetInfo>(in_string);
            if (!deserializedTargetInfo)
            {
                throw std::runtime_error(fmt::format("Failed to deserialize json: {}", deserializedTargetInfo.error().what()));
            }

            auto targetInfo = std::make_unique<rdma::TargetInfo>(deserializedTargetInfo.value());
            *out_targetInfo = targetInfo.release()->toAPI();

            return MXL_STATUS_OK;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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

        return MXL_ERR_INVALID_ARG;
    }

    extern "C" MXL_EXPORT
    mxlStatus mxlFabricsTargetInfoToString(mxlTargetInfo const in_targetInfo, char* out_string, size_t* in_stringSize)
    {
        if (in_targetInfo == nullptr || in_stringSize == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            std::stringstream ss;
            auto targetInfo = rdma::TargetInfo::fromAPI(in_targetInfo);
            auto targetInfoString = rfl::json::write(targetInfo);

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
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
        if (in_info == nullptr)
        {
            return MXL_ERR_INVALID_ARG;
        }

        try
        {
            auto info = rdma::TargetInfo::fromAPI(in_info);

            delete info;
        }
        catch (rdma::Exception& e)
        {
            MXL_ERROR("Failed to create regions object: {}", e.what());

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
}
