#pragma once

#include <cstdint>
#include <variant>

namespace mxl::lib::fabrics::ofi
{

    class DataLayout
    {
    public:
        struct VideoDataLayout
        {
            bool alphaPresent;
        };

        struct AudioDataLayout
        {
            std::uint32_t channelCount;
            std::uint32_t samplesPerChannel;
            std::size_t bytesPerSample;
        };

    public:
        static DataLayout fromVideo(bool alphaPresent) noexcept;
        static DataLayout fromAudio(std::uint32_t channelCount, std::uint32_t samplesPerChannel, std::size_t bytesPerSample) noexcept;

        [[nodiscard]]
        bool isVideo() const noexcept;
        [[nodiscard]]
        bool isAudio() const noexcept;

        [[nodiscard]]
        VideoDataLayout const& asVideo() const noexcept;
        [[nodiscard]]
        AudioDataLayout const& asAudio() const noexcept;

    private:
        using InnerLayout = std::variant<VideoDataLayout, AudioDataLayout>;

    private:
        DataLayout(InnerLayout) noexcept;

    private:
        InnerLayout _inner;
    };

}
