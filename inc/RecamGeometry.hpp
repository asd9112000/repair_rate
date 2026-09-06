#ifndef RECAM_GEOMETRY_HPP
#define RECAM_GEOMETRY_HPP

#include <cstdint>
#include <optional>

namespace dynamic_spare
{

// Technology-independent RECAM geometry. CAM and SRAM backends must derive
// their entry counts and logical field widths from this single definition.
struct RecamGeometryConfig
{
    std::uint32_t rows = 1024;
    std::uint32_t columns = 1024;
    std::uint32_t spareRows = 2;
    std::uint32_t spareColumns = 2;
    std::uint32_t channels = 1;
    std::uint32_t dataWordBits = 64;
    std::optional<std::uint32_t> onlineReuseEntries;

    void validate() const;
};

struct RecamGeometry
{
    std::uint64_t k = 0;
    std::uint64_t addressEntries = 0;
    std::uint64_t hybridEntries = 0;
    std::uint64_t runtimeEntries = 0;
    std::uint64_t tempEntries = 0;

    std::uint32_t rowAddressBits = 0;
    std::uint32_t columnAddressBits = 0;
    std::uint32_t channelAddressBits = 0;
    std::uint32_t pointerBits = 0;
    std::uint32_t lineAddressBits = 0;
    std::uint32_t rowFailCounterBits = 0;
    std::uint32_t columnFailCounterBits = 0;

    std::uint64_t addressOfflineEntryBits = 0;
    std::uint64_t addressOnlineEntryBits = 0;
    std::uint64_t addressEntryBits = 0;
    std::uint64_t hybridOfflineEntryBits = 0;
    std::uint64_t hybridOnlineEntryBits = 0;
    std::uint64_t hybridEntryBits = 0;
    std::uint64_t tempEntryBits = 0;
    std::uint64_t runtimeKeyBits = 0;
    std::uint64_t runtimeEntryBits = 0;

    std::uint64_t addressLogicalBits = 0;
    std::uint64_t hybridLogicalBits = 0;
    std::uint64_t tempBufferBits = 0;
    std::uint64_t coreLogicalBits = 0;
    std::uint64_t totalStorageBits = 0;
    std::uint64_t runtimeCompactBits = 0;
    std::uint64_t matrixBits = 0;
    std::uint64_t solutionUnits = 0;
};

RecamGeometry deriveRecamGeometry(const RecamGeometryConfig &config);

} // namespace dynamic_spare

#endif
