#include "../inc/RecamGeometry.hpp"

#include <algorithm>
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

std::uint64_t checkedMultiply(
    std::uint64_t left,
    std::uint64_t right,
    const char *description)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error(description);
    }
    return left * right;
}

std::uint32_t ceilLog2(std::uint64_t value)
{
    if (value <= 1)
    {
        return 0;
    }
    std::uint32_t bits = 0;
    std::uint64_t representable = 1;
    while (representable < value)
    {
        if (representable > std::numeric_limits<std::uint64_t>::max() / 2)
        {
            throw std::overflow_error("RECAM address width overflow");
        }
        representable <<= 1;
        ++bits;
    }
    return bits;
}

std::uint64_t choose(std::uint64_t total, std::uint64_t selected)
{
    selected = std::min(selected, total - selected);
    unsigned __int128 result = 1;
    for (std::uint64_t index = 1; index <= selected; ++index)
    {
        result = result * (total - selected + index) / index;
        if (result > std::numeric_limits<std::uint64_t>::max())
        {
            throw std::overflow_error(
                "RECAM solution count exceeds uint64 range");
        }
    }
    return static_cast<std::uint64_t>(result);
}

} // namespace

void RecamGeometryConfig::validate() const
{
    if (rows == 0 || columns == 0 ||
        (spareRows == 0 && spareColumns == 0) ||
        channels == 0 || dataWordBits == 0)
    {
        throw std::invalid_argument(
            "RECAM dimensions, channels, and word width must be positive, "
            "and Rs/Cs cannot both be zero");
    }
    if (minimumAddressEntryBits.has_value() &&
        *minimumAddressEntryBits == 0)
    {
        throw std::invalid_argument(
            "Minimum Address CAM entry width must be positive when configured");
    }
}

RecamGeometry deriveRecamGeometry(const RecamGeometryConfig &config)
{
    config.validate();
    RecamGeometry result;
    result.k = checkedAdd(
        config.spareRows, config.spareColumns, "RECAM K overflow");
    result.addressEntries = result.k;
    result.hybridEntries =
        config.spareRows == 0 || config.spareColumns == 0
            ? 0
            : checkedAdd(
                  checkedMultiply(
                      config.spareRows,
                      config.spareColumns - 1,
                      "RECAM Hybrid entry count overflow"),
                  checkedMultiply(
                      config.spareColumns,
                      config.spareRows - 1,
                      "RECAM Hybrid entry count overflow"),
                  "RECAM Hybrid entry count overflow");
    result.tempEntries = config.onlineReuseEntries.value_or(
        static_cast<std::uint32_t>(result.k));
    result.runtimeEntries = result.tempEntries;

    result.rowAddressBits = ceilLog2(config.rows);
    result.columnAddressBits = ceilLog2(config.columns);
    result.channelAddressBits = config.channels <= 1
        ? 0
        : ceilLog2(config.channels);
    result.pointerBits = std::max<std::uint32_t>(1, ceilLog2(result.k));
    result.lineAddressBits = std::max(
        result.rowAddressBits, result.columnAddressBits);
    result.rowFailCounterBits = std::max<std::uint32_t>(
        1, ceilLog2(static_cast<std::uint64_t>(config.spareColumns) + 1));
    result.columnFailCounterBits = std::max<std::uint32_t>(
        1, ceilLog2(static_cast<std::uint64_t>(config.spareRows) + 1));

    result.addressOfflineEntryBits =
        1 + result.rowAddressBits + result.columnAddressBits + 1 + 1 +
        result.rowFailCounterBits + result.columnFailCounterBits;
    result.addressOnlineEntryBits =
        1 + result.rowAddressBits + result.columnAddressBits +
        result.channelAddressBits;
    result.addressEntryBits = std::max(
        result.addressOfflineEntryBits, result.addressOnlineEntryBits);
    if (config.minimumAddressEntryBits.has_value())
    {
        result.addressEntryBits = std::max(
            result.addressEntryBits, *config.minimumAddressEntryBits);
    }
    result.hybridOfflineEntryBits =
        1 + result.pointerBits + 1 + result.lineAddressBits;
    result.hybridOnlineEntryBits =
        1 + result.pointerBits + config.dataWordBits;
    result.hybridEntryBits = std::max(
        result.hybridOfflineEntryBits, result.hybridOnlineEntryBits);
    result.tempEntryBits = result.rowAddressBits +
        result.columnAddressBits + result.channelAddressBits;
    result.runtimeKeyBits = result.tempEntryBits;
    result.runtimeEntryBits =
        1 + result.runtimeKeyBits + config.dataWordBits;

    result.addressLogicalBits = checkedMultiply(
        result.addressEntries,
        result.addressEntryBits,
        "Address storage logical bit count overflow");
    result.hybridLogicalBits = checkedMultiply(
        result.hybridEntries,
        result.hybridEntryBits,
        "Hybrid storage logical bit count overflow");
    result.tempBufferBits = checkedMultiply(
        result.tempEntries,
        result.tempEntryBits,
        "Temporary buffer bit count overflow");
    result.coreLogicalBits = checkedAdd(
        result.addressLogicalBits,
        result.hybridLogicalBits,
        "Core RECAM logical bit count overflow");
    result.totalStorageBits = checkedAdd(
        result.coreLogicalBits,
        result.tempBufferBits,
        "Total RECAM storage bit count overflow");
    result.runtimeCompactBits = checkedMultiply(
        result.runtimeEntries,
        result.runtimeEntryBits,
        "Runtime compact storage bit count overflow");
    result.matrixBits = checkedMultiply(
        result.k, result.k, "RECAM matrix size overflow");
    result.solutionUnits = choose(result.k, config.spareRows);
    return result;
}

} // namespace dynamic_spare
