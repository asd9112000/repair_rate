#include "FaultAddress.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

Fault fault(int subarray, int row, int column)
{
    Fault result{};
    result.SubarrayID = subarray;
    result.r = row;
    result.c = column;
    return result;
}

void verifyWordMaskConversion()
{
    dynamic_spare::FaultAddressGeometry geometry;
    dynamic_spare::BistFaultPacket packet;
    packet.address = {0, 0, 0, 0, 2, 10, 5};
    packet.failMask.assign(256, false);
    packet.failMask[17] = true;
    packet.failMask[255] = true;

    const std::vector<Fault> physical =
        dynamic_spare::decodeBistFaultPacket(packet, geometry);
    require(physical.size() == 2 && physical[0].c == 1297 &&
                physical[1].c == 1535,
            "Word+mask packet did not decode to physical cell columns");
    const auto encoded = dynamic_spare::encodeBistWordAddress(
        physical[0], geometry);
    require(encoded.wordColumn == 5 && encoded.row == 10 &&
                dynamic_spare::bitOffsetWithinWord(
                    physical[0], geometry) == 17,
            "Physical address did not round-trip to word+syndrome");
}

void verifySerialAbcdSchedule()
{
    dynamic_spare::SerialBistSchedule schedule;
    require(schedule.arrivalCycle(fault(0, 0, 0)) == 1,
            "Subarray A first word arrival is wrong");
    require(schedule.arrivalCycle(fault(0, 511, 8191)) == 16384,
            "Subarray A last word arrival is wrong");
    require(schedule.arrivalCycle(fault(1, 0, 0)) == 16385,
            "Subarray B did not start after A");
    require(schedule.arrivalCycle(fault(2, 10, 1297)) == 33094,
            "Subarray C example arrival is wrong");
    require(schedule.arrivalCycle(fault(3, 511, 8191)) == 65536 &&
                schedule.completionCycle() == 65536,
            "Subarray D/BIST completion boundary is wrong");
    require(schedule.arrivalCycle(fault(2, 10, 1297)) ==
                schedule.arrivalCycle(fault(2, 10, 1535)),
            "Physical faults in one word received different arrivals");
}

void verifyInvalidMaskRejected()
{
    dynamic_spare::FaultAddressGeometry geometry;
    dynamic_spare::BistFaultPacket packet;
    packet.address = {0, 0, 0, 0, 0, 0, 0};
    packet.failMask.assign(256, false);
    bool rejected = false;
    try
    {
        (void)dynamic_spare::decodeBistFaultPacket(packet, geometry);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "Empty BIST fail mask was accepted");
}

} // namespace

int main()
{
    try
    {
        verifyWordMaskConversion();
        verifySerialAbcdSchedule();
        verifyInvalidMaskRejected();
        std::cout << "Fault-address/BIST schedule tests passed\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Fault-address/BIST schedule test failed: "
                  << error.what() << '\n';
        return 1;
    }
}
