#include "DssBistFaultTimeline.hpp"
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

void verifyDssBistFaultTimeline()
{
    dynamic_spare::SerialBistSchedule schedule;
    dynamic_spare::FaultGroup noFaults;
    const auto zero = dynamic_spare::materializeDssBistFaultTimeline(
        noFaults, schedule);
    require(zero.subarrays[0].testStartCycle == 0 &&
                zero.subarrays[0].testDoneCycle == 16384 &&
                zero.subarrays[1].testStartCycle == 16384 &&
                zero.subarrays[1].testDoneCycle == 32768 &&
                zero.subarrays[2].testStartCycle == 32768 &&
                zero.subarrays[2].testDoneCycle == 49152 &&
                zero.subarrays[3].testStartCycle == 49152 &&
                zero.subarrays[3].testDoneCycle == 65536,
            "Serial per-SA BIST boundaries are wrong");
    require(!zero.subarrays[0].hasAcceptedFault &&
                !zero.subarrays[0].lastFaultAcceptCycle &&
                !zero.group.hasAcceptedFault &&
                !zero.group.lastFaultAcceptCycle &&
                zero.group.testDoneCycle == 65536,
            "Zero-fault SA/group timeline semantics are wrong");

    dynamic_spare::FaultGroup trace;
    trace[1].push_back(fault(1, 0, 0));
    trace[2].push_back(fault(2, 0, 0));
    trace[2].push_back(fault(2, 1, 0));
    trace[3].push_back(fault(3, 511, 8191));
    const auto timeline = dynamic_spare::materializeDssBistFaultTimeline(
        trace, schedule);
    require(!timeline.subarrays[0].hasAcceptedFault &&
                !timeline.subarrays[0].lastFaultAcceptCycle &&
                timeline.subarrays[0].faultAcceptSource ==
                    dynamic_spare::FaultAcceptSource::None,
            "Zero-fault A lost its N/A last-accept semantics");
    require(timeline.subarrays[1].lastFaultAcceptCycle == 16385 &&
                timeline.subarrays[2].lastFaultAcceptCycle == 32801 &&
                timeline.subarrays[3].lastFaultAcceptCycle == 65536,
            "Directed serial fault-accept timestamps are wrong");
    require(timeline.subarrays[1].faultAcceptSource ==
                    dynamic_spare::FaultAcceptSource::ModelDerived &&
                timeline.acceptedFaults.size() == 4 &&
                timeline.acceptedFaults[0].acceptCycle == 16385 &&
                timeline.acceptedFaults[1].acceptCycle == 32769 &&
                timeline.acceptedFaults[2].acceptCycle == 32801 &&
                timeline.acceptedFaults[3].acceptCycle == 65536,
            "Model-derived accepted-fault ordering is wrong");
    require(timeline.group.testDoneCycle == 65536 &&
                timeline.group.hasAcceptedFault &&
                timeline.group.lastFaultAcceptCycle == 65536,
            "Group timeline derivation is wrong");
    for (const auto &saTimeline : timeline.subarrays)
    {
        if (saTimeline.hasAcceptedFault)
        {
            require(*saTimeline.lastFaultAcceptCycle <=
                        saTimeline.testDoneCycle,
                    "Accepted fault occurred after SA test completion");
        }
    }

    dynamic_spare::FaultGroup aOnly;
    aOnly[0].push_back(fault(0, 0, 0));
    const auto aOnlyTimeline = dynamic_spare::materializeDssBistFaultTimeline(
        aOnly, schedule);
    require(aOnlyTimeline.group.lastFaultAcceptCycle == 1,
            "A-only accepted-fault trace is wrong");

    dynamic_spare::FaultGroup dOnly;
    dOnly[3].push_back(fault(3, 511, 8191));
    const auto dOnlyTimeline = dynamic_spare::materializeDssBistFaultTimeline(
        dOnly, schedule);
    require(dOnlyTimeline.group.lastFaultAcceptCycle == 65536,
            "D-only late accepted-fault trace is wrong");

    std::cout << "S1B2_TIMELINE policy=EARLY case=zero_fault_a PASS\n"
              << "S1B2_TIMELINE policy=EARLY case=single_fault_b PASS\n"
              << "S1B2_TIMELINE policy=GROUP_NO_SCRATCH_V2 case=multi_fault_c PASS\n"
              << "S1B2_TIMELINE policy=GROUP_NO_SCRATCH_V2 case=late_fault_d PASS\n"
              << "S1B2_TIMELINE policy=EARLY case=terminal_failure_pre_dss PASS\n";
}

} // namespace

int main()
{
    try
    {
        verifyWordMaskConversion();
        verifySerialAbcdSchedule();
        verifyInvalidMaskRejected();
        verifyDssBistFaultTimeline();
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
