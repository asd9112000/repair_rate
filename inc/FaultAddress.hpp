#ifndef FAULT_ADDRESS_HPP
#define FAULT_ADDRESS_HPP

#include "Fault.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dynamic_spare
{

// External BIST port address. Col is a word column. This type is deliberately
// distinct from legacy Fault, whose c is always a physical cell column.
struct BistWordAddress7
{
    int HBMID = 0;
    int ChannelID = 0;
    int BankID = 0;
    int SubarrayGroupID = 0;
    int SubarrayID = 0;
    std::uint32_t row = 0;
    std::uint32_t wordColumn = 0;
};

struct BistFaultPacket
{
    BistWordAddress7 address;
    // Exactly wordBits bits. Every set bit becomes one physical cell fault.
    std::vector<bool> failMask;
};

struct FaultAddressGeometry
{
    std::uint32_t rows = 512;
    std::uint32_t cellColumns = 8192;
    std::uint32_t wordBits = 256;

    void validate() const;
    std::uint32_t wordsPerRow() const;
};

std::vector<Fault> decodeBistFaultPacket(
    const BistFaultPacket &packet,
    const FaultAddressGeometry &geometry);

BistWordAddress7 encodeBistWordAddress(
    const Fault &physicalFault,
    const FaultAddressGeometry &geometry);

std::uint32_t bitOffsetWithinWord(
    const Fault &physicalFault,
    const FaultAddressGeometry &geometry);

// Group-relative BIST baseline: subarrays are scanned strictly A, B, C, D;
// each subarray scans Row-major then WordCol-major.
struct SerialBistSchedule
{
    FaultAddressGeometry geometry;
    std::uint32_t subarrayCount = 4;
    std::uint64_t cyclesPerWord = 1;
    std::uint64_t groupStartCycle = 0;

    void validate() const;
    std::uint64_t arrivalCycle(const Fault &physicalFault) const;
    std::uint64_t completionCycle() const;
};

} // namespace dynamic_spare

#endif
