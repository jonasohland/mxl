// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#include "Initiator.hpp"
#include <algorithm>
#include <mxl-internal/Logging.hpp>
#include "mxl/fabrics.h"
#include "CompletionQueue.hpp"
#include "Exception.hpp"
#include "FabricInterfaceProbe.hpp"
#include "ProtocolEgressRMA.hpp"
#include "RCInitiator.hpp"
#include "RDMInitiator.hpp"
#include "Region.hpp"
#include "VerbsDevice.hpp"

namespace mxl::lib::fabrics::ofi
{
    namespace
    {
        /** \brief Select the source interface for a continuous flow initiator.
         *
         * With the verbs provider, request a tx iov_limit large enough to post the scatter-gather list of a sample transfer as a
         * single write, capped at the max_sge that libibverbs reports for the device. Without the hint the verbs provider uses
         * FI_VERBS_TX_IOV_LIMIT (default 4).
         */
        std::pair<FabricInfo, ProviderConfig> selectContinuousSourceInterface(::mxlFabricsInterfaceConfig const& interfaceConfig,
            std::size_t iovCount)
        {
            auto selected = selectSourceInterface(interfaceConfig, /* target */ false);
            if (interfaceConfig.provider != MXL_FABRICS_PROVIDER_VERBS)
            {
                return selected;
            }

            auto const maxSge = queryVerbsMaxSge(selected.first->domain_attr->name);
            if (!maxSge)
            {
                MXL_WARN("Could not query max_sge for device '{}', using the provider default iov_limit of {}",
                    selected.first->domain_attr->name,
                    selected.first.view().txIovLimit());
                return selected;
            }

            auto const iovLimit = std::min(iovCount, *maxSge);
            if (iovLimit <= selected.first.view().txIovLimit())
            {
                return selected;
            }

            return selectSourceInterface(interfaceConfig, /* target */ false, iovLimit);
        }

        /** \brief Return the initiator options with the completion queue depth scaled for continuous flow transfers.
         *
         * Endpoint::write() splits a scatter-gather list into ceil(iovCount / iovLimit) writes, and each write produces one
         * completion. The configured or default depth is multiplied by that count so that the queue holds the same number of
         * outstanding sample transfers as it would if every transfer were a single write.
         */
        InitiatorSetupOptions scaleOptionsForContinuousFlow(InitiatorSetupOptions options, std::size_t iovCount, std::size_t iovLimit)
        {
            auto const writesPerTransfer = (iovCount + iovLimit - 1) / iovLimit;
            if (writesPerTransfer > 1)
            {
                auto const baseDepth = options.cqDepth.value_or(CompletionQueue::Attributes::DEFAULT_SIZE);
                options.cqDepth = baseDepth * writesPerTransfer;
                MXL_INFO("Sample transfers need up to {} iovs with an iov_limit of {}, posting {} writes per transfer. "
                         "Increasing the completion queue depth from {} to {}.",
                    iovCount,
                    iovLimit,
                    writesPerTransfer,
                    baseDepth,
                    *options.cqDepth);
            }
            return options;
        }
    }

    InitiatorWrapper* InitiatorWrapper::fromAPI(mxlFabricsInitiator api) noexcept
    {
        return reinterpret_cast<InitiatorWrapper*>(api);
    }

    mxlFabricsInitiator InitiatorWrapper::toAPI() noexcept
    {
        return reinterpret_cast<mxlFabricsInitiator>(this);
    }

    void InitiatorWrapper::setup(mxlFabricsInitiatorConfig const& config, InitiatorSetupOptions const& options)
    {
        if (_inner)
        {
            _inner.reset();
        }

        auto const layout = MxlRegions::forReader(config.reader).dataLayout();
        if (!layout.isContinuous())
        {
            auto const [info, providerConfig] = selectSourceInterface(config.interface, /* target */ false);
            setupInner(config, info.view(), options);
            return;
        }

        auto const iovCount = maxSampleTransferIovCount(layout.asContinuous());
        auto const [info, providerConfig] = selectContinuousSourceInterface(config.interface, iovCount);
        setupInner(config, info.view(), scaleOptionsForContinuousFlow(options, iovCount, info.view().txIovLimit()));
    }

    void InitiatorWrapper::setupInner(mxlFabricsInitiatorConfig const& config, FabricInfoView info, InitiatorSetupOptions const& options)
    {
        switch (info.endpointType())
        {
            case FI_EP_MSG: _inner = RCInitiator::setup(config, info, options); break;
            case FI_EP_RDM: _inner = RDMInitiator::setup(config, info, options); break;
            default:        throw Exception::invalidState("unsupported endpoint type");
        }
    }

    void InitiatorWrapper::addTarget(TargetInfo const& targetInfo)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        _inner->addTarget(targetInfo);
    }

    void InitiatorWrapper::removeTarget(TargetInfo const& targetInfo)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        _inner->removeTarget(targetInfo);
    }

    void InitiatorWrapper::transferGrain(std::uint64_t grainIndex, std::uint16_t startSlice, std::uint16_t endSlice)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        _inner->transferGrain(grainIndex, startSlice, endSlice);
    }

    void InitiatorWrapper::transferGrainToTarget(Endpoint::Id targetId, std::uint64_t localIndex, std::uint64_t remoteIndex,
        std::uint64_t payloadOffset, std::uint16_t startSlice, std::uint16_t endSlice)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        _inner->transferGrainToTarget(targetId, localIndex, remoteIndex, payloadOffset, startSlice, endSlice);
    }

    void InitiatorWrapper::transferSamples(std::uint64_t headIndex, std::size_t count)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        _inner->transferSamples(headIndex, count);
    }

    Initiator::MakeProgressResult InitiatorWrapper::makeProgress()
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        return _inner->makeProgress();
    }

    Initiator::MakeProgressResult InitiatorWrapper::makeProgressBlocking(std::chrono::steady_clock::duration timeout)
    {
        if (!_inner)
        {
            throw Exception::invalidState("Initiator is not set up");
        }

        return _inner->makeProgressBlocking(timeout);
    }

}
