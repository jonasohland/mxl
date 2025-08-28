#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include "mxl/fabrics.h"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::rdma_core
{
    class Target
    {
    public:
        struct ReadResult
        {
            std::optional<std::uint64_t> grainAvailable{std::nullopt};
        };

        virtual ~Target() = default;

        virtual ReadResult read() = 0;
        virtual ReadResult readBlocking(std::chrono::steady_clock::duration timeout) = 0;
    };

    class TargetWrapper
    {
    public:
        static TargetWrapper* fromAPI(mxlFabricsTarget) noexcept;
        mxlFabricsTarget toAPI() noexcept;

        Target::ReadResult read();
        Target::ReadResult readBlocking(std::chrono::steady_clock::duration timeout);

        std::unique_ptr<TargetInfo> setup(mxlTargetConfig const&);

    private:
        std::unique_ptr<Target> _inner;
    };

}
