#include "../inc/FaultAddress.hpp"

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

void validatePhysicalFault(
    const Fault &fault,
    const FaultAddressGeometry &geometry)
{
    if (fault.r < 0 || fault.c < 0 ||
        static_cast<std::uint32_t>(fault.r) >= geometry.rows ||
        static_cast<std::uint32_t>(fault.c) >= geometry.cellColumns)
    {
        throw std::invalid_argument(
            "Physical fault is outside the address geometry");
    }
}

} // namespace

void FaultAddressGeometry::validate() const
{
    if (rows == 0 || cellColumns == 0 || wordBits == 0)
    {
        throw std::invalid_argument(
            "Fault-address rows, columns, and word width must be positive");
    }
    if (cellColumns % wordBits != 0)
    {
        throw std::invalid_argument(
            "Physical cell columns must be divisible by wordBits");
    }
}

std::uint32_t FaultAddressGeometry::wordsPerRow() const
{
    validate();
    return cellColumns / wordBits;
}

std::vector<Fault> decodeBistFaultPacket(
    const BistFaultPacket &packet,
    const FaultAddressGeometry &geometry)
{
    geometry.validate();
    if (packet.address.HBMID < 0 || packet.address.ChannelID < 0 ||
        packet.address.BankID < 0 ||
        packet.address.SubarrayGroupID < 0 ||
        packet.address.SubarrayID < 0)
    {
        throw std::invalid_argument("BIST word address IDs cannot be negative");
    }
    if (packet.address.row >= geometry.rows ||
        packet.address.wordColumn >= geometry.wordsPerRow())
    {
        throw std::invalid_argument(
            "BIST word address is outside the configured memory");
    }
    if (packet.failMask.size() != geometry.wordBits)
    {
        throw std::invalid_argument(
            "BIST fault mask width does not equal wordBits");
    }

    std::vector<Fault> faults;
    for (std::uint32_t bit = 0; bit < geometry.wordBits; ++bit)
    {
        if (!packet.failMask[bit])
        {
            continue;
        }
        Fault fault{};
        fault.HBMID = packet.address.HBMID;
        fault.ChannelID = packet.address.ChannelID;
        fault.BankID = packet.address.BankID;
        fault.SubarrayGroupID = packet.address.SubarrayGroupID;
        fault.SubarrayID = packet.address.SubarrayID;
        fault.r = static_cast<int>(packet.address.row);
        const std::uint64_t physicalColumn = checkedAdd(
            checkedMultiply(
                packet.address.wordColumn,
                geometry.wordBits,
                "BIST physical column overflow"),
            bit,
            "BIST physical column overflow");
        if (physicalColumn > static_cast<std::uint64_t>(
                std::numeric_limits<int>::max()))
        {
            throw std::overflow_error(
                "BIST physical column exceeds legacy Fault int range");
        }
        fault.c = static_cast<int>(physicalColumn);
        fault.faultType = FaultType::Cell;
        faults.push_back(fault);
    }
    if (faults.empty())
    {
        throw std::invalid_argument("BIST fault packet has an empty fail mask");
    }
    return faults;
}

BistWordAddress7 encodeBistWordAddress(
    const Fault &physicalFault,
    const FaultAddressGeometry &geometry)
{
    geometry.validate();
    validatePhysicalFault(physicalFault, geometry);
    return {
        physicalFault.HBMID,
        physicalFault.ChannelID,
        physicalFault.BankID,
        physicalFault.SubarrayGroupID,
        physicalFault.SubarrayID,
        static_cast<std::uint32_t>(physicalFault.r),
        static_cast<std::uint32_t>(physicalFault.c) / geometry.wordBits};
}

std::uint32_t bitOffsetWithinWord(
    const Fault &physicalFault,
    const FaultAddressGeometry &geometry)
{
    geometry.validate();
    validatePhysicalFault(physicalFault, geometry);
    return static_cast<std::uint32_t>(physicalFault.c) % geometry.wordBits;
}

void SerialBistSchedule::validate() const
{
    geometry.validate();
    if (subarrayCount == 0 || cyclesPerWord == 0)
    {
        throw std::invalid_argument(
            "Serial BIST subarray count and cycles per word must be positive");
    }
}

std::uint64_t SerialBistSchedule::arrivalCycle(
    const Fault &physicalFault) const
{
    validate();
    validatePhysicalFault(physicalFault, geometry);
    if (physicalFault.SubarrayID < 0 ||
        static_cast<std::uint32_t>(physicalFault.SubarrayID) >= subarrayCount)
    {
        throw std::invalid_argument(
            "Physical fault SubarrayID is outside the serial BIST group");
    }
    const std::uint64_t words = geometry.wordsPerRow();
    const std::uint64_t wordColumn =
        static_cast<std::uint32_t>(physicalFault.c) / geometry.wordBits;
    const std::uint64_t subarrayAndRow = checkedAdd(
        checkedMultiply(
            static_cast<std::uint32_t>(physicalFault.SubarrayID),
            geometry.rows,
            "Serial BIST scan index overflow"),
        static_cast<std::uint32_t>(physicalFault.r),
        "Serial BIST scan index overflow");
    const std::uint64_t scanIndex = checkedAdd(
        checkedMultiply(
            subarrayAndRow, words, "Serial BIST scan index overflow"),
        wordColumn,
        "Serial BIST scan index overflow");
    return checkedAdd(
        groupStartCycle,
        checkedMultiply(
            checkedAdd(scanIndex, 1, "Serial BIST scan index overflow"),
            cyclesPerWord,
            "Serial BIST arrival-cycle overflow"),
        "Serial BIST arrival-cycle overflow");
}

std::uint64_t SerialBistSchedule::completionCycle() const
{
    validate();
    const std::uint64_t addressCount = checkedMultiply(
        checkedMultiply(
            subarrayCount,
            geometry.rows,
            "Serial BIST address-count overflow"),
        geometry.wordsPerRow(),
        "Serial BIST address-count overflow");
    return checkedAdd(
        groupStartCycle,
        checkedMultiply(
            addressCount,
            cyclesPerWord,
            "Serial BIST completion-cycle overflow"),
        "Serial BIST completion-cycle overflow");
}

} // namespace dynamic_spare
