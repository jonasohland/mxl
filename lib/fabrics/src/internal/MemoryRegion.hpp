#include <cstdint>

class MemoryRegion
{
public:
    MemoryRegion(std::uint8_t buf, std::size_t size);
    MemoryRegion(MemoryRegion&& other);

    MemoryRegion(MemoryRegion const&) = delete;

private:
    std::uint8_t buf;
    std::size_t size;
};
