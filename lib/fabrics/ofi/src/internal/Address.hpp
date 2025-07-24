#pragma once

#include <vector>
#include <rfl.hpp>
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <sys/types.h>

namespace mxl::lib::fabrics::ofi
{
    class FabricAddress final
    {
    public:
        explicit FabricAddress();

        static FabricAddress fromFid(::fid_t fid)
        {
            return retrieveFabricAddress(fid);
        }

        [[nodiscard]]
        std::string toBase64() const;

        static FabricAddress fromBase64(std::string_view data);

        void* raw();

        [[nodiscard]]
        void* raw() const;

    private:
        explicit FabricAddress(std::vector<uint8_t> addr);
        static FabricAddress retrieveFabricAddress(::fid_t);

        std::vector<uint8_t> _inner;
    };

    struct FabricAddressRfl
    {
        rfl::Field<"addr", std::string> addr;

        static FabricAddressRfl from_class(FabricAddress const& fa)
        {
            return FabricAddressRfl{.addr = fa.toBase64()};
        }

        [[nodiscard]]
        FabricAddress to_class() const
        {
            return FabricAddress::fromBase64(addr.get());
        }
    };
}

namespace rfl::parsing
{
    namespace ofi = mxl::lib::fabrics::ofi;

    template<class ReaderType, class WriterType, class ProcessorsType>
    struct Parser<ReaderType, WriterType, ofi::FabricAddress, ProcessorsType>
        : public rfl::parsing::CustomParser<ReaderType, WriterType, ProcessorsType, ofi::FabricAddress, ofi::FabricAddressRfl>
    {};
}
