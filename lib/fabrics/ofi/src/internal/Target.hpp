#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <mxl/fabrics.h>
#include <mxl/mxl.h>
#include "Domain.hpp"
#include "Endpoint.hpp"
#include "MemoryRegion.hpp"
#include "PassiveEndpoint.hpp"
#include "TargetInfo.hpp"

namespace mxl::lib::fabrics::ofi
{
    class Target
    {
    public:
        virtual ~Target() = default;

        [[nodiscard]]
        virtual TargetInfo getInfo() const = 0;
    };

    struct TargetProgressResult
    {
        std::optional<uint64_t> grainCompleted{std::nullopt};
    };

    class TargetWrapper
    {
    public:
        TargetWrapper() = default;

        ~TargetWrapper() = default;

        static TargetWrapper* fromAPI(mxlFabricsTarget api) noexcept;
        mxlFabricsTarget toAPI() noexcept;

        std::pair<mxlStatus, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const& config) noexcept;

        TargetProgressResult tryGrain();
        TargetProgressResult waitForGrain(std::chrono::steady_clock::duration timeout);

    private:
        struct StateFresh
        {};

        struct StateWaitConnReq

        {
            std::shared_ptr<PassiveEndpoint> pep;
        };

        struct StateWaitForConnected
        {
            std::shared_ptr<Endpoint> ep;
        };

        struct StateConnected
        {
            std::shared_ptr<Endpoint> ep;
        };

        using State = std::variant<StateFresh, StateWaitConnReq, StateWaitForConnected, StateConnected>;

        TargetProgressResult doProgress();
        TargetProgressResult doProgressBlocking(std::chrono::steady_clock::duration timeout);

        std::unique_ptr<Target> _inner;
        std::optional<std::shared_ptr<Domain>> _domain = std::nullopt;
        std::vector<RegisteredRegion> _regions;

        State _state = StateFresh{};
    };
}
