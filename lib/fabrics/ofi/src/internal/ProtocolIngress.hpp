#pragma once

#include "BounceBuffer.hpp"
#include "DataLayout.hpp"
#include "Domain.hpp"
#include "LocalRegion.hpp"
#include "Protocol.hpp"

namespace mxl::lib::fabrics::ofi
{
    class IngressProtocolWriter final : public IngressProtocol
    {
    public:
        IngressProtocolWriter() = default;

        void processCompletion(std::uint32_t immData) final;
    };

    class IngressProtocolWriterWithBounceBuffer final : public IngressProtocol
    {
    public:
        IngressProtocolWriterWithBounceBuffer(std::shared_ptr<Domain> domain, DataLayout::AudioDataLayout layout, Region dstRegion);

        void processCompletion(std::uint32_t immData) final;

    private:
        std::shared_ptr<Domain> _domain;
        BounceBuffer _bounceBuffer;
        LocalRegion _dstRegion;
    };

    class IngressProtocolReceiverDiscrete final : public IngressProtocol

    {
        IngressProtocolReceiverDiscrete(DataLayout::VideoDataLayout layout, std::shared_ptr<std::vector<LocalRegion>> localRegions);

        void processCompletion(std::uint32_t immData) final;

    private:
        BounceBuffer _bounceBuffer;
        std::shared_ptr<std::vector<LocalRegion>> _localRegions;
    };

    class IngressProtocolReceiverContinuous final : public IngressProtocol
    {
    public:
        IngressProtocolReceiverContinuous(DataLayout::AudioDataLayout layout, std::shared_ptr<LocalRegion> localRegion);

        void processCompletion(std::uint32_t immData) final;

    private:
        BounceBuffer _bounceBuffer;
        std::shared_ptr<LocalRegion> _localRegion;
    };

}
