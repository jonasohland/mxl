#pragma once

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>
#include <uuid.h>
#include <bits/types/struct_iovec.h>
#include <rdma/fi_domain.h>
#include "internal/FlowData.hpp"
#include "mxl/fabrics.h"
#include "Domain.hpp"

namespace mxl::lib::fabrics::ofi
{
    class RegisteredRegion;
    class RegisteredRegionGroup;

    class Region
    {
    public:
        class Location
        {
        public:
            static Location host() noexcept;
            static Location cuda(int deviceId) noexcept;
            static Location fromAPI(mxlFabricsMemoryRegionLocation loc) noexcept;

            [[nodiscard]]
            uint64_t id() const noexcept;

            [[nodiscard]]
            ::fi_hmem_iface iface() const noexcept;

            [[nodiscard]]
            std::string toString() const noexcept;

            [[nodiscard]]
            bool isHost() const noexcept;

        private:
            class Host
            {};

            class Cuda
            {
                friend class Location;

                Cuda(int deviceId)
                    : _deviceId(deviceId)
                {}

                int _deviceId;
            };

            using Variant = std::variant<Host, Cuda>;

            Location(Variant inner)
                : _inner(inner)
            {}

            Variant _inner;
        };

        explicit Region(std::uintptr_t base, size_t size, Location loc = Location::host())
            : base(base)
            , size(size)
            , loc(loc)
            , _iovec(iovecFromRegion(base, size))
        {}

        virtual ~Region() = default;

        [[nodiscard]]
        RegisteredRegion reg(std::shared_ptr<Domain> domain, uint64_t access) const;

        std::uintptr_t base;
        size_t size;
        Location loc;

        [[nodiscard]]
        ::iovec const* as_iovec() const noexcept;

        [[nodiscard]]
        ::iovec to_iovec() const noexcept;

    private:
        static ::iovec iovecFromRegion(std::uintptr_t, size_t) noexcept;

        ::iovec _iovec;
    };

    class RegionGroup

    {
    public:
        explicit RegionGroup() = default;

        explicit RegionGroup(std::vector<Region> inner)
            : _inner(std::move(inner))
            , _iovecs(iovecsFromGroup(_inner))
        {}

        [[nodiscard]]
        RegisteredRegionGroup reg(std::shared_ptr<Domain> domain, uint64_t access) const;

        [[nodiscard]]
        std::vector<Region> const& view() const noexcept;

        [[nodiscard]]
        ::iovec const* as_iovec() const noexcept;

    private:
        static std::vector<::iovec> iovecsFromGroup(std::vector<Region> const& group) noexcept;

        std::vector<Region> _inner;

        std::vector<::iovec> _iovecs;
    };

    class RegionGroups
    {
    public:
        static RegionGroups fromFlow(FlowData& flow);
        static RegionGroups fromGroups(mxlFabricsMemoryRegionGroup const* groups, size_t count);

        static RegionGroups* fromAPI(mxlRegions) noexcept;
        [[nodiscard]]
        mxlRegions toAPI() noexcept;

        [[nodiscard]]
        std::vector<RegisteredRegionGroup> reg(std::shared_ptr<Domain> domain, uint64_t access) const;

        [[nodiscard]]
        std::vector<RegionGroup> const& view() const noexcept;

    private:
        explicit RegionGroups(std::vector<RegionGroup> inner)
            : _inner(std::move(inner))
        {}

        std::vector<RegionGroup> _inner;
    };

}
