#include "../inc/HardwareMetrics.hpp"

#include <limits>
#include <stdexcept>

namespace dynamic_spare
{
namespace
{

std::uint64_t checkedAdd(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error(description);
    }
    return left + right;
}

} // namespace

void finalizeHardwareMetrics(HardwareMetrics &metrics)
{
    metrics.totalStorageBits = checkedAdd(
        metrics.addressPhysicalBits,
        checkedAdd(
            metrics.hybridPhysicalBits,
            metrics.temporaryPhysicalBits,
            "Hardware storage-bit total overflow"),
        "Hardware storage-bit total overflow");
    if (!metrics.offlineOnlineModeReused)
    {
        metrics.totalStorageBits = checkedAdd(
            metrics.totalStorageBits,
            metrics.runtimePhysicalBits,
            "Hardware storage-bit total overflow");
    }
    metrics.totalModeledBits = checkedAdd(
        metrics.totalStorageBits,
        checkedAdd(
            metrics.matrixBits,
            metrics.comparatorBits,
            "Hardware modeled-bit total overflow"),
        "Hardware modeled-bit total overflow");
}

} // namespace dynamic_spare
