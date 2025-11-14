// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include "Endpoint.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{

    struct InitiatorTargetEntry
    {
        Endpoint endpoint;
        std::vector<RemoteRegionGroup> remoteGroups;
    };

    class Initiator
    {
    public:
        virtual ~Initiator() = default;

        virtual void addTarget(TargetInfo const& targetInfo) = 0;
        virtual void removeTarget(TargetInfo const& targetInfo) = 0;
        virtual void transferGrain(std::uint64_t grainIndex, std::uint64_t offset, std::uint32_t size, std::uint16_t validSlices) = 0;

        virtual void transferGrainToTarget(Endpoint::Id targetId, std::uint64_t localIndex, std::uint64_t localOffset, std::uint64_t remoteIndex,
            std::uint64_t remoteOffset, std::uint32_t size, std::uint16_t validSlices) = 0;

        virtual bool makeProgress() = 0;
        virtual bool makeProgressBlocking(std::chrono::steady_clock::duration) = 0;
    };

    class InitiatorWrapper
    {
    public:
        static InitiatorWrapper* fromAPI(mxlFabricsInitiator api) noexcept;
        mxlFabricsInitiator toAPI() noexcept;

        void setup(mxlInitiatorConfig const& config);

        void addTarget(TargetInfo const& targetInfo);
        void removeTarget(TargetInfo const& targetInfo);
        void transferGrain(std::uint64_t grainIndex, std::uint64_t offset, std::uint32_t size, std::uint16_t validSlices);
        void transferGrainToTarget(Endpoint::Id targetId, std::uint64_t localIndex, std::uint64_t localOffset, std::uint64_t remoteIndex,
            std::uint64_t remoteOffset, std::uint32_t size, std::uint16_t validSlices);
        bool makeProgress();
        bool makeProgressBlocking(std::chrono::steady_clock::duration);

    private:
        std::unique_ptr<Initiator> _inner;
    };
}
