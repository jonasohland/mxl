#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <mxl/fabrics.h>
#include <mxl/mxl.h>
#include "internal/Instance.hpp"
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
        virtual ~Target()
        {}

        [[nodiscard]]
        virtual TargetInfo getInfo() const = 0;
    };

    class TargetWrapper
    {
    public:
        TargetWrapper(Instance* mxlInstance);
        ~TargetWrapper();

        std::pair<mxlStatus, std::unique_ptr<TargetInfo>> setup(mxlTargetConfig const& config) noexcept;

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

        void doProgress();
        void doProgressBlocking(std::chrono::steady_clock::duration timeout);

        std::unique_ptr<Target> _inner;
        std::optional<std::shared_ptr<Domain>> _domain = std::nullopt;
        std::optional<std::shared_ptr<MemoryRegion>> _mr = std::nullopt;

        State _state = StateFresh{};
        Instance* _mxlInstance;
    };
}
