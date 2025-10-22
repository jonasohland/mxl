// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "FlowData.hpp"

namespace mxl::lib
{
    ///
    /// Simple structure holding the shared memory resources of discrete flows.
    ///
    class ContinuousFlowData : public FlowData
    {
    public:
        explicit ContinuousFlowData(SharedMemoryInstance<Flow>&& flowSegement) noexcept;
        ContinuousFlowData(char const* flowFilePath, AccessMode mode);

        constexpr std::size_t channelCount() const noexcept;
        constexpr std::size_t sampleWordSize() const noexcept;
        constexpr std::size_t channelBufferLength() const noexcept;

        void openChannelBuffers(char const* channelBuffersFilePath, std::size_t sampleWordSize);

        /** The size of the mapped channel data in bytes. */
        constexpr std::size_t channelDataSize() const noexcept;

        /** The length of the mapped channel data in samples. */
        constexpr std::size_t channelDataLength() const noexcept;

        constexpr void* channelData() noexcept;
        constexpr void const* channelData() const noexcept;

        void multiBufferSlices(std::uint64_t index, std::size_t count, mxlMutableWrappedMultiBufferSlice& slices) noexcept;
        void multiBufferSlices(std::uint64_t index, std::size_t count, mxlWrappedMultiBufferSlice& slices) const noexcept;

    private:
        SharedMemorySegment _channelBuffers;
        std::size_t _sampleWordSize;
    };

    /**************************************************************************/
    /* Inline implementation.                                                 */
    /**************************************************************************/

    inline ContinuousFlowData::ContinuousFlowData(SharedMemoryInstance<Flow>&& flowSegement) noexcept
        : FlowData{std::move(flowSegement)}
        , _channelBuffers{}
        , _sampleWordSize{1U}
    {}

    inline ContinuousFlowData::ContinuousFlowData(char const* flowFilePath, AccessMode mode)
        : FlowData{flowFilePath, mode}
        , _channelBuffers{}
        , _sampleWordSize{1U}
    {}

    constexpr std::size_t ContinuousFlowData::channelCount() const noexcept
    {
        auto const info = flowInfo();
        return (info != nullptr) ? info->continuous.channelCount : 0U;
    }

    constexpr std::size_t ContinuousFlowData::channelBufferLength() const noexcept
    {
        auto const info = flowInfo();
        return (info != nullptr) ? info->continuous.bufferLength : 0U;
    }

    constexpr std::size_t ContinuousFlowData::sampleWordSize() const noexcept
    {
        return _sampleWordSize;
    }

    inline void ContinuousFlowData::openChannelBuffers(char const* grainFilePath, std::size_t sampleWordSize)
    {
        if ((sampleWordSize != 0U) || !created())
        {
            auto const info = flowInfo();
            auto const channelCount = info->continuous.channelCount;
            auto const bufferLength = info->continuous.bufferLength;
            if (auto const buffersLength = channelCount * bufferLength; buffersLength > 0U)
            {
                auto const mode = this->created() ? AccessMode::CREATE_READ_WRITE : this->accessMode();
                _channelBuffers = SharedMemorySegment{grainFilePath, mode, buffersLength * sampleWordSize};

                auto const mappedSize = _channelBuffers.mappedSize();
                _sampleWordSize = (sampleWordSize != 0U) ? sampleWordSize : ((mappedSize >= buffersLength) ? (mappedSize / buffersLength) : 1U);
            }
            else
            {
                throw std::runtime_error{"Attempt to open channel buffer with invalid geometry."};
            }
        }
        else
        {
            throw std::runtime_error{"Attempt to create channel buffer with invalid sample word size."};
        }
    }

    constexpr std::size_t ContinuousFlowData::channelDataSize() const noexcept
    {
        return _channelBuffers.mappedSize();
    }

    constexpr std::size_t ContinuousFlowData::channelDataLength() const noexcept
    {
        return _channelBuffers.mappedSize() / _sampleWordSize;
    }

    constexpr void* ContinuousFlowData::channelData() noexcept
    {
        return _channelBuffers.data();
    }

    constexpr void const* ContinuousFlowData::channelData() const noexcept
    {
        return _channelBuffers.data();
    }

    // TODO: merge these two
    inline void getMultiBufferSlices(std::uint64_t index, std::size_t count, size_t channelBufferLength, std::size_t sampleWordSize,
        std::size_t channelCount, std::uint8_t const* baseBufferPtr, mxlWrappedMultiBufferSlice& slice) noexcept
    {
        auto const startOffset = (index + channelBufferLength - count) % channelBufferLength;
        auto const endOffset = (index % channelBufferLength);

        auto const firstLength = (startOffset < endOffset) ? count : channelBufferLength - startOffset;
        auto const secondLength = count - firstLength;

        slice.base.fragments[0].pointer = baseBufferPtr + sampleWordSize * startOffset;
        slice.base.fragments[0].size = sampleWordSize * firstLength;

        slice.base.fragments[1].pointer = baseBufferPtr;
        slice.base.fragments[1].size = sampleWordSize * secondLength;

        slice.stride = sampleWordSize * channelBufferLength;
        slice.count = channelCount;
    }

    inline void getMultiBufferSlices(std::uint64_t index, std::size_t count, size_t channelBufferLength, std::size_t sampleWordSize,
        std::size_t channelCount, std::uint8_t* baseBufferPtr, mxlMutableWrappedMultiBufferSlice& slice) noexcept
    {
        auto const startOffset = (index + channelBufferLength - count) % channelBufferLength;
        auto const endOffset = (index % channelBufferLength);

        auto const firstLength = (startOffset < endOffset) ? count : channelBufferLength - startOffset;
        auto const secondLength = count - firstLength;

        slice.base.fragments[0].pointer = baseBufferPtr + sampleWordSize * startOffset;
        slice.base.fragments[0].size = sampleWordSize * firstLength;

        slice.base.fragments[1].pointer = baseBufferPtr;
        slice.base.fragments[1].size = sampleWordSize * secondLength;

        slice.stride = sampleWordSize * channelBufferLength;
        slice.count = channelCount;
    }

    // TODO: merge these two

    inline void ContinuousFlowData::multiBufferSlices(std::uint64_t index, std::size_t count, mxlMutableWrappedMultiBufferSlice& slices) noexcept
    {
        getMultiBufferSlices(
            index, count, channelBufferLength(), sampleWordSize(), channelCount(), static_cast<std::uint8_t*>(channelData()), slices);
    }

    inline void ContinuousFlowData::multiBufferSlices(std::uint64_t index, std::size_t count, mxlWrappedMultiBufferSlice& slices) const noexcept
    {
        getMultiBufferSlices(
            index, count, channelBufferLength(), sampleWordSize(), channelCount(), static_cast<std::uint8_t const*>(channelData()), slices);
    }

}
