#include "FIInfo.hpp"
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "Exception.hpp"
#include "FIVersion.hpp"

namespace mxl::lib::fabrics::ofi
{
    FIInfo::FIInfo(::fi_info const* raw) noexcept
        : _raw(::fi_dupinfo(raw))
    {}

    FIInfo::~FIInfo() noexcept
    {
        free();
    }

    FIInfo::FIInfo(FIInfo const& other) noexcept
        : _raw(::fi_dupinfo(other._raw))
    {}

    void FIInfo::operator=(FIInfo const& other) noexcept
    {
        free();

        _raw = ::fi_dupinfo(other._raw);
    }

    FIInfo::FIInfo(FIInfo&& other) noexcept
        : _raw(other._raw)
    {
        other._raw = nullptr;
    }

    FIInfo& FIInfo::operator=(FIInfo&& other) noexcept
    {
        _raw = other._raw;
        other._raw = nullptr;

        return *this;
    }

    ::fi_info& FIInfo::operator*() noexcept
    {
        return *_raw;
    }

    ::fi_info const& FIInfo::operator*() const noexcept
    {
        return *_raw;
    }

    ::fi_info* FIInfo::raw() noexcept
    {
        return _raw;
    }

    ::fi_info const* FIInfo::raw() const noexcept
    {
        return _raw;
    }

    void FIInfo::free() noexcept
    {
        if (_raw != nullptr)
        {
            ::fi_freeinfo(_raw);
            _raw = nullptr;
        }
    }

    FIInfoView::FIInfoView(::fi_info* raw)
        : _raw(raw)
    {}

    ::fi_info& FIInfoView::operator*() noexcept
    {
        return *_raw;
    }

    ::fi_info const& FIInfoView::operator*() const noexcept
    {
        return *_raw;
    }

    ::fi_info* FIInfoView::operator->() noexcept
    {
        return _raw;
    }

    ::fi_info const* FIInfoView::operator->() const noexcept
    {
        return _raw;
    }

    ::fi_info* FIInfoView::raw() noexcept
    {
        return _raw;
    }

    ::fi_info const* FIInfoView::raw() const noexcept
    {
        return _raw;
    }

    FIInfo FIInfoView::owned() noexcept
    {
        return {_raw};
    }

    FIInfoList FIInfoList::get(std::string node, std::string service)
    {
        ::fi_info* info;

        fiCall(::fi_getinfo, "Failed to get provider information", fiVersion(), node.c_str(), service.c_str(), 0, nullptr, &info);

        return FIInfoList{info};
    }

    FIInfoList::FIInfoList(::fi_info* begin) noexcept
        : _begin(begin)
    {}

    FIInfoList::~FIInfoList()
    {
        free();
    }

    FIInfoList::FIInfoList(FIInfoList&& other) noexcept
        : _begin(other._begin)
    {
        other._begin = nullptr;
    }

    FIInfoList& FIInfoList::operator=(FIInfoList&& other) noexcept
    {
        free();

        _begin = other._begin;
        other._begin = nullptr;
        return *this;
    }

    FIInfoList::iterator FIInfoList::begin() noexcept
    {
        return iterator{_begin};
    }

    FIInfoList::iterator FIInfoList::end() noexcept
    {
        return iterator{nullptr};
    }

    FIInfoList::const_iterator FIInfoList::begin() const noexcept
    {
        return const_iterator{_begin};
    }

    FIInfoList::const_iterator FIInfoList::end() const noexcept
    {
        return const_iterator{nullptr};
    }

    FIInfoList::const_iterator FIInfoList::cbegin() const noexcept
    {
        return const_iterator{_begin};
    }

    FIInfoList::const_iterator FIInfoList::cend() const noexcept
    {
        return const_iterator{nullptr};
    }

    void FIInfoList::free()
    {
        if (_begin != nullptr)
        {
            ::fi_freeinfo(_begin);
            _begin = nullptr;
        }
    }
}
