#include "DataLayout.hpp"
#include <variant>

namespace mxl::lib::fabrics::ofi
{
    DataLayout DataLayout::fromVideo(bool alphaPresent) noexcept
    {
        return DataLayout{VideoDataLayout{.alphaPresent = alphaPresent}};
    }

    DataLayout DataLayout::fromAudio(std::uint32_t channelCount, std::uint32_t samplesPerChannel, std::size_t bytesPerSample) noexcept
    {
        return DataLayout{
            AudioDataLayout{.channelCount = channelCount, .samplesPerChannel = samplesPerChannel, .bytesPerSample = bytesPerSample}
        };
    }

    bool DataLayout::isVideo() const noexcept
    {
        return std::holds_alternative<VideoDataLayout>(_inner);
    }

    bool DataLayout::isAudio() const noexcept
    {
        return std::holds_alternative<AudioDataLayout>(_inner);
    }

    DataLayout::VideoDataLayout const& DataLayout::asVideo() const noexcept
    {
        return std::get<VideoDataLayout>(_inner);
    }

    DataLayout::AudioDataLayout const& DataLayout::asAudio() const noexcept
    {
        return std::get<AudioDataLayout>(_inner);
    }

    DataLayout::DataLayout(InnerLayout inner) noexcept
        : _inner(inner)
    {}
}
