#pragma once

#include <cstddef>
#include <iterator>
#include <rdma/fabric.h>
#include <type_traits>
#include "internal/Logging.hpp"

namespace mxl::lib::fabrics::ofi
{

    class FIInfoList;

    /**
     * Implements a const/non-const ForwardIterator over a ::fi_info linked list,
     * for use with range-base for-loops and c++20 ranges.
     */
    template<bool Const>
    class FIInfoIterator
    {
    public:
        friend FIInfoList;

        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<Const, ::fi_info const*, ::fi_info*>;

        value_type operator*() const
        {
            return _it;
        }

        FIInfoIterator& operator++()
        {
            _it = _it->next;
            return *this;
        }

        FIInfoIterator operator++(int)
        {
            auto current = _it;
            ++*this;
            return FIInfoIterator{current};
        }

        bool operator==(FIInfoIterator const& other)
        {
            return other._it == _it;
        }

        bool operator!=(FIInfoIterator const& other)
        {
            return !(*this == other);
        }

    private:
        explicit FIInfoIterator(value_type it) noexcept
            : _it(it)
        {}

        value_type _it;
    };

    class FIInfoList
    {
    public:
        /**
         * Get a list of provider configurations supported to the specified
         * node/service
         */
        static FIInfoList get(std::string node, std::string service);

        // Type aliases for const and non-const versions of the iterator template
        using iterator = FIInfoIterator<false>;
        using const_iterator = FIInfoIterator<true>;

        // calls ::fi_freeinfo to deallocate the underlying linked-list
        ~FIInfoList();

        // deleted copy constuctor
        FIInfoList(FIInfoList const&) = delete;
        FIInfoList& operator=(FIInfoList const&) = delete;

        // move semantics
        FIInfoList(FIInfoList&& other) noexcept;
        FIInfoList& operator=(FIInfoList&& other) noexcept;

        // non-const forward iterator
        iterator begin() noexcept;
        iterator end() noexcept;

        // const forward iterator
        [[nodiscard]]
        const_iterator begin() const noexcept;
        [[nodiscard]]
        const_iterator end() const noexcept;

        // const forward iterator
        [[nodiscard]]
        const_iterator cbegin() const noexcept;
        [[nodiscard]]
        const_iterator cend() const noexcept;

    private:
        explicit FIInfoList(::fi_info*) noexcept;
        void free();

        ::fi_info* _begin;
    };

}
