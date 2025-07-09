#include "FIInfo.hpp"
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include "mxl/mxl.h"
#include "Exception.hpp"
#include "FIVersion.hpp"

namespace mxl::lib::fabrics::ofi
{
    FIInfoList FIInfoList::get(std::string node, std::string service)
    {
        ::fi_info* info;

        auto res = ::fi_getinfo(fiVersion(), node.c_str(), service.c_str(), 0, nullptr, &info);
        if (res != FI_SUCCESS)
        {
            throw Exception::make(MXL_ERR_UNKNOWN, "Failed to get provider info: code {}", res);
        }

        return FIInfoList{info};
    }

    FIInfoList::FIInfoList(::fi_info* begin) noexcept
        : _begin(begin)
    {}

    FIInfoList::~FIInfoList()
    {
        if (_begin != nullptr)
        {
            ::fi_freeinfo(_begin);
            _begin = nullptr;
        }
    }

    FIInfoList::FIInfoList(FIInfoList&& other) noexcept
        : _begin(other._begin)
    {
        other._begin = nullptr;
    }

    FIInfoList& FIInfoList::operator=(FIInfoList&& other) noexcept
    {
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
}
