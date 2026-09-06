#ifndef HARDWARE_METRICS_HPP
#define HARDWARE_METRICS_HPP

#include "BiraLatency.hpp"
#include "RecamGeometry.hpp"

#include <cstdint>

namespace dynamic_spare
{

// Common first-order hardware accounting contract. It reports storage and
// comparator bits separately; it is not a foundry macro-area claim.
struct HardwareMetrics
{
    BiraStorageTechnology storageTechnology = BiraStorageTechnology::Cam;
    RecamGeometry geometry;

    std::uint64_t addressPhysicalBits = 0;
    std::uint64_t hybridPhysicalBits = 0;
    std::uint64_t temporaryPhysicalBits = 0;
    std::uint64_t runtimePhysicalBits = 0;
    std::uint64_t matrixBits = 0;
    std::uint64_t comparatorBits = 0;

    // When true, runtimePhysicalBits describes an alternative online view and
    // is not added again to totalStorageBits.
    bool offlineOnlineModeReused = true;
    std::uint64_t totalStorageBits = 0;
    std::uint64_t totalModeledBits = 0;
};

void finalizeHardwareMetrics(HardwareMetrics &metrics);

} // namespace dynamic_spare

#endif
