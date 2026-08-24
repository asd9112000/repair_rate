#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "DynamicCsvReporter.hpp"
#include "DynamicFaultGenerator.hpp"
#include "DynamicRepairSimulator.hpp"
#include "Fault.hpp"
#include "PhysicalResourceLedger.hpp"
#include "RepairResult.hpp"
#include "SimulationConfig.hpp"

namespace
{

using dynamic_spare::FaultGroup;

void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

std::size_t choose(std::size_t total, std::size_t selected)
{
    selected = std::min(selected, total - selected);
    std::size_t value = 1;
    for (std::size_t index = 1; index <= selected; ++index)
    {
        value = value * (total - selected + index) / index;
    }
    return value;
}

bool sameFault(const Fault &left, const Fault &right)
{
    return left.r == right.r && left.c == right.c &&
           left.HBMID == right.HBMID &&
           left.ChannelID == right.ChannelID &&
           left.BankID == right.BankID &&
           left.SubarrayGroupID == right.SubarrayGroupID &&
           left.SubarrayID == right.SubarrayID &&
           left.faultType == right.faultType;
}

bool sameFaultGroup(const FaultGroup &left, const FaultGroup &right)
{
    for (std::size_t subarray = 0;
         subarray < dynamic_spare::kSubarrayCount; ++subarray)
    {
        if (left[subarray].size() != right[subarray].size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left[subarray].size(); ++index)
        {
            if (!sameFault(left[subarray][index], right[subarray][index]))
            {
                return false;
            }
        }
    }
    return true;
}

std::array<std::size_t, 4> faultCounts(const FaultGroup &group)
{
    return {{
        group[0].size(), group[1].size(),
        group[2].size(), group[3].size()}};
}

void verifyFaultGenerator()
{
    dynamic_spare::SimulationConfig config;
    config.faultCount = 20;
    config.simulationRuns = 2;
    config.randomSeed = 77123;
    config.memoryRows = 64;
    config.memoryColumns = 64;

    const std::array<std::pair<dynamic_spare::FaultCountModel,
                               std::array<std::size_t, 4>>, 4> cases{{
        {dynamic_spare::FaultCountModel::Uniform, {{5, 5, 5, 5}}},
        {dynamic_spare::FaultCountModel::ModerateImbalance, {{2, 4, 5, 9}}},
        {dynamic_spare::FaultCountModel::StrongImbalance, {{1, 2, 3, 14}}},
        {dynamic_spare::FaultCountModel::Hotspot, {{1, 1, 1, 17}}}}};

    for (const auto &testCase : cases)
    {
        config.faultCountModel = testCase.first;
        dynamic_spare::DynamicFaultGenerator first(config);
        dynamic_spare::DynamicFaultGenerator second(config);
        const FaultGroup firstGroup = first.generate(0);
        const FaultGroup secondGroup = second.generate(0);
        require(faultCounts(firstGroup) == testCase.second,
                "Fault-count model does not preserve the requested group total");
        require(sameFaultGroup(firstGroup, secondGroup),
                "Fixed-seed fault generation is not deterministic");

        for (std::size_t subarray = 0; subarray < 4; ++subarray)
        {
            std::set<std::pair<int, int>> addresses;
            for (const Fault &fault : firstGroup[subarray])
            {
                require(fault.SubarrayID == static_cast<int>(subarray),
                        "Generated fault has the wrong SubarrayID");
                require(addresses.emplace(fault.r, fault.c).second,
                        "Generated fault addresses are not unique");
            }
        }
    }

    config.faultCountModel = dynamic_spare::FaultCountModel::UserDefined;
    config.userDefinedFaultCounts = {{2, 4, 5, 9}};
    dynamic_spare::DynamicFaultGenerator userDefined(config);
    require(faultCounts(userDefined.generate(0)) ==
                std::array<std::size_t, 4>{{2, 4, 5, 9}},
            "User-defined fault counts changed");
}

void verifyDirectionalLedger()
{
    dynamic_spare::SimulationConfig config;
    config.topology = dynamic_spare::SharingTopology::Directional;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.modifiers.maximumGroupBorrowedSpares = 3;
    dynamic_spare::PhysicalResourceLedger ledger(config);

    const std::array<dynamic_spare::SpareDemand, 4> demands{{
        {1, 3}, {2, 1}, {3, 1}, {1, 2}}};
    const auto allocation = ledger.allocate(demands);
    require(allocation.success, "Valid directional allocation failed");
    require(allocation.physicalRows == 8 &&
                allocation.physicalColumns == 8,
            "Directional policy changed physical capacity");
    require(allocation.usedRows == 7 && allocation.usedColumns == 7 &&
                allocation.unusedRows == 1 &&
                allocation.unusedColumns == 1,
            "Directional utilization does not conserve physical lines");
    require(allocation.borrowedRows() == 1 &&
                allocation.borrowedColumns() == 1 &&
                allocation.transfers.size() == 2,
            "Directional borrow count is incorrect");

    bool sawAToCRow = false;
    bool sawBToAColumn = false;
    std::set<std::size_t> physicalIds;
    for (const auto &transfer : allocation.transfers)
    {
        require(physicalIds.insert(transfer.physicalLineId).second,
                "One physical spare was lent more than once");
        sawAToCRow = sawAToCRow ||
            (transfer.donorSubarray == 0 &&
             transfer.borrowerSubarray == 2 &&
             transfer.dimension == dynamic_spare::SpareDimension::Row);
        sawBToAColumn = sawBToAColumn ||
            (transfer.donorSubarray == 1 &&
             transfer.borrowerSubarray == 0 &&
             transfer.dimension == dynamic_spare::SpareDimension::Column);
    }
    require(sawAToCRow && sawBToAColumn,
            "Confirmed directional ownership mapping changed");

    config.modifiers.minimumRowReserve = 1;
    config.modifiers.minimumColumnReserve = 1;
    const auto reserved =
        dynamic_spare::PhysicalResourceLedger(config).allocate(demands);
    require(!reserved.success && reserved.failedBorrows == 2 &&
                reserved.donorStarvationCount == 2,
            "Minimum reserve did not block lending the last free spare");

    config.modifiers.minimumRowReserve = 0;
    config.modifiers.minimumColumnReserve = 0;
    const std::array<dynamic_spare::SpareDemand, 4> wrongDirection{{
        {3, 2}, {2, 2}, {2, 2}, {2, 2}}};
    require(!dynamic_spare::PhysicalResourceLedger(config)
                 .allocate(wrongDirection).success,
            "Directional policy allowed A to borrow a row");
}

void verifyGlobalAndEdgeLedgers()
{
    dynamic_spare::SimulationConfig global;
    global.topology = dynamic_spare::SharingTopology::GlobalPool;
    global.sharedRows = 1;
    global.sharedColumns = 1;
    global.globalPool = dynamic_spare::GlobalPoolConfiguration{1, 1, 4, 4};
    global.modifiers.maximumGroupBorrowedSpares = 3;
    const std::array<dynamic_spare::SpareDemand, 4> globalDemands{{
        {2, 2}, {1, 1}, {1, 1}, {1, 1}}};
    const auto globalAllocation =
        dynamic_spare::PhysicalResourceLedger(global).allocate(globalDemands);
    require(globalAllocation.success &&
                globalAllocation.globalRowsUsed == 1 &&
                globalAllocation.globalColumnsUsed == 1 &&
                globalAllocation.remainingGlobalRows == 3 &&
                globalAllocation.remainingGlobalColumns == 3,
            "Global-pool accounting is incorrect");
    require(globalAllocation.usedRows + globalAllocation.unusedRows == 8 &&
                globalAllocation.usedColumns +
                    globalAllocation.unusedColumns == 8,
            "Global pool violates physical conservation");

    global.modifiers.singleDimensionBorrowing = true;
    require(!dynamic_spare::PhysicalResourceLedger(global)
                 .allocate(globalDemands).success,
            "Single-dimension modifier allowed one subarray to borrow R and C");

    dynamic_spare::SimulationConfig edge;
    edge.topology = dynamic_spare::SharingTopology::PairwiseEdge;
    edge.sharedRows = 1;
    edge.sharedColumns = 1;
    edge.modifiers.maximumGroupBorrowedSpares = 3;
    const std::array<dynamic_spare::SpareDemand, 4> edgeDemands{{
        {2, 3}, {1, 1}, {1, 2}, {3, 2}}};
    const auto edgeAllocation =
        dynamic_spare::PhysicalResourceLedger(edge).allocate(edgeDemands);
    require(edgeAllocation.success && edgeAllocation.transfers.size() == 2,
            "Valid edge allocation failed");
    for (const auto &transfer : edgeAllocation.transfers)
    {
        const std::size_t donor = static_cast<std::size_t>(
            transfer.donorSubarray);
        const std::size_t borrower = transfer.borrowerSubarray;
        const bool legalRow =
            transfer.dimension == dynamic_spare::SpareDimension::Row &&
            ((donor == 0 && borrower == 2) ||
             (donor == 2 && borrower == 0) ||
             (donor == 1 && borrower == 3) ||
             (donor == 3 && borrower == 1));
        const bool legalColumn =
            transfer.dimension == dynamic_spare::SpareDimension::Column &&
            ((donor == 0 && borrower == 1) ||
             (donor == 1 && borrower == 0) ||
             (donor == 2 && borrower == 3) ||
             (donor == 3 && borrower == 2));
        require(legalRow || legalColumn,
                "Edge topology performed a diagonal/non-edge transfer");
    }
}

std::uint64_t attemptCycleComponentSum(
    const dynamic_spare::RepairAttemptResult &attempt)
{
    return attempt.latency.faultInformationInsertCycles +
           attempt.latency.faultInformationLookupCycles +
           attempt.latency.faultInformationReadCycles +
           attempt.latency.matrixGenerationCycles +
           attempt.latency.solutionGenerationCycles +
           attempt.latency.solutionEvaluationCycles +
           attempt.latency.sharingAllocationCycles;
}

std::size_t attemptCount(const dynamic_spare::GroupRepairResult &group)
{
    std::size_t total = 0;
    for (const auto &attempts : group.attemptsBySubarray)
    {
        total += attempts.size();
    }
    return total;
}

void verifySimulatorMetrics()
{
    dynamic_spare::SimulationConfig config;
    config.faultCount = 8;
    config.simulationRuns = 1;
    config.randomSeed = 99181;
    config.memoryRows = 64;
    config.memoryColumns = 64;
    config.hybridCamEntryWidthBits = 64;
    config.latency.cam = {2, 3, 4};
    config.latency.sram = {5, 7, 11};
    config.latency.matrixGenerationCyclesPerCell = 13;
    config.latency.solutionGenerationCyclesPerCandidate = 17;
    config.latency.solutionEvaluationCyclesPerCandidate = 19;
    config.latency.sharingAllocationCyclesPerRequest = 23;

    dynamic_spare::DynamicFaultGenerator generator(config);
    const FaultGroup faults = generator.generate(0);
    dynamic_spare::DynamicRepairSimulator simulator;
    const auto baseline = simulator.run(faults, config, 0);
    const auto repeated = simulator.run(faults, config, 0);
    require(attemptCount(baseline) == 4 &&
                baseline.extraAnalysisAttempts == 0,
            "No-sharing baseline did not perform exactly four attempts");
    require(baseline.groupRepairSuccess == repeated.groupRepairSuccess &&
                baseline.latency.totalCycles() == repeated.latency.totalCycles(),
            "Repeated simulation is not deterministic");

    std::uint64_t expectedGroupCycles = 0;
    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        const auto &attempt = baseline.attemptsBySubarray[subarray].front();
        require(attempt.candidateSolutions == choose(
                    attempt.matrixDimension,
                    static_cast<std::size_t>(attempt.availableRows)),
                "Candidate count is not choose(R+C,R)");
        require(attempt.candidateSolutionsEvaluated ==
                    attempt.candidateSolutions &&
                    attempt.failedCandidates +
                        attempt.validCandidateIndices.size() ==
                    attempt.candidateSolutions,
                "Candidate accounting does not conserve the solution space");
        require(attempt.latency.totalCycles() ==
                    attemptCycleComponentSum(attempt),
                "Attempt total cycle sum is inconsistent");
        require(attempt.addressCamEntriesPeak.value() >=
                    attempt.addressCamEntriesActive &&
                    attempt.hybridCamEntriesPeak.value() >=
                    attempt.hybridCamEntriesActive,
                "Peak CAM usage is below final active usage");
        require(attempt.hybridCamBitsProvisioned ==
                    attempt.hybridCamEntriesProvisioned * 64,
                "Hybrid-CAM bit cost ignored direct entry width");
        require(attempt.bufferCamEntriesProvisioned == 4,
                "Dynamic simulator default is not Rs+Cs CAM reuse");
        require(attempt.validCandidateIndices ==
                    repeated.attemptsBySubarray[subarray]
                        .front().validCandidateIndices,
                "Candidate order changed between repeated runs");
        expectedGroupCycles += attempt.latency.totalCycles();
    }
    require(baseline.latency.totalCycles() == expectedGroupCycles,
            "Group cycle sum is not the sum of attempt components");

    dynamic_spare::SimulationConfig exhaustive = config;
    exhaustive.topology = dynamic_spare::SharingTopology::Directional;
    exhaustive.sharedRows = 1;
    exhaustive.sharedColumns = 1;
    exhaustive.modifiers.maximumGroupBorrowedSpares = 2;
    exhaustive.modifiers.localFirst = false;
    const auto exhaustiveResult = simulator.run(faults, exhaustive, 0);
    require(attemptCount(exhaustiveResult) == 8,
            "Exhaustive directional mode did not analyze all capacities");

    dynamic_spare::SimulationConfig localFirst = exhaustive;
    localFirst.modifiers.localFirst = true;
    const auto localFirstResult = simulator.run(faults, localFirst, 0);
    require(attemptCount(localFirstResult) <= attemptCount(exhaustiveResult),
            "Local-first increased the number of analysis attempts");
    if (localFirstResult.baselineGroupRepairSuccess)
    {
        require(attemptCount(localFirstResult) == 4,
                "Local-first did not stop after successful local repair");
    }

    std::uint64_t sharingCycles =
        localFirstResult.latency.sharingAllocationCycles;
    for (const auto &attempts : localFirstResult.attemptsBySubarray)
    {
        for (const auto &attempt : attempts)
        {
            sharingCycles += attempt.latency.totalCycles();
        }
    }
    require(localFirstResult.latency.totalCycles() == sharingCycles,
            "Sharing cycle total does not equal its components");
    require(localFirstResult.sharing.borrowRequests ==
                localFirstResult.sharing.successfulBorrows +
                    localFirstResult.sharing.failedBorrows,
            "Borrow request accounting is not conserved");
    if (localFirstResult.groupRepairSuccess)
    {
        require(localFirstResult.usedRows +
                    localFirstResult.unusedPhysicalRows == 8 &&
                    localFirstResult.usedColumns +
                    localFirstResult.unusedPhysicalColumns == 8,
                "Successful group violates spare conservation");
        require(localFirstResult.sharing.borrowedRows +
                    localFirstResult.sharing.borrowedColumns ==
                    localFirstResult.sharing.lentRows +
                    localFirstResult.sharing.lentColumns,
                "Borrowed and lent physical counts differ");
    }

    dynamic_spare::SimulationConfig sram = config;
    sram.storageMode = dynamic_spare::FaultInformationStorage::SRAM;
    const auto sramResult = simulator.run(faults, sram, 0);
    require(sramResult.groupRepairSuccess == baseline.groupRepairSuccess &&
                sramResult.latency.totalCycles() !=
                    baseline.latency.totalCycles(),
            "Configurable SRAM latency changed repair logic or was ignored");

    dynamic_spare::SimulationConfig zeroAxisGlobal = config;
    zeroAxisGlobal.faultCount = 0;
    zeroAxisGlobal.topology = dynamic_spare::SharingTopology::GlobalPool;
    zeroAxisGlobal.sharedRows = 2;
    zeroAxisGlobal.sharedColumns = 2;
    zeroAxisGlobal.globalPool =
        dynamic_spare::GlobalPoolConfiguration{0, 0, 8, 8};
    zeroAxisGlobal.modifiers.maximumGroupBorrowedSpares = 3;
    dynamic_spare::DynamicFaultGenerator zeroGenerator(zeroAxisGlobal);
    const auto zeroResult = simulator.run(
        zeroGenerator.generate(0), zeroAxisGlobal, 0);
    require(zeroResult.groupRepairSuccess,
            "sharedLine=2 zero-local-axis empty group should be repairable");
}

std::vector<FaultGroup> loadGoldenFixture()
{
    FaultLoader loader(2, 2, 2);
    require(loader.loadFaults(
                "tests/fixtures/dynamic_sharing_seed_20260820.faults"),
            "Unable to load golden fault fixture");
    require(loader.faultLists.size() == 128,
            "Golden fixture subarray count changed");
    std::vector<FaultGroup> groups(32);
    for (std::size_t run = 0; run < 32; ++run)
    {
        for (std::size_t subarray = 0; subarray < 4; ++subarray)
        {
            groups[run][subarray] =
                loader.faultLists[run * 4 + subarray].PEFaults;
        }
    }
    return groups;
}

std::string readFile(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    require(input.is_open(), "Unable to read generated CSV");
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

void verifyGoldenRunnerAndCsv()
{
    const auto groups = loadGoldenFixture();
    dynamic_spare::SimulationConfig config;
    config.faultCount = 28;
    config.simulationRuns = 32;
    config.randomSeed = 20260820;
    config.hybridCamEntryWidthBits = 64;
    // The locked SharedLine golden used its historical fixed two-entry buffer.
    config.bufferCamEntries = 2;
    config.usePaperCamReuseCapacity = false;
    dynamic_spare::DynamicRepairSimulator simulator;
    dynamic_spare::SimulationBatch batch;
    batch.config = config;
    std::size_t successes = 0;
    for (std::size_t run = 0; run < groups.size(); ++run)
    {
        auto result = simulator.run(groups[run], config, run);
        result.baselineGroupRepairSuccess = result.groupRepairSuccess;
        result.baselineAnalysisCycles = result.latency.totalCycles();
        successes += result.groupRepairSuccess ? 1 : 0;
        batch.runs.push_back(std::move(result));
    }
    require(successes == 10,
            "New baseline runner changed golden repair rate 10/32");

    const auto policySuccesses = [&](dynamic_spare::SimulationConfig policy)
    {
        std::size_t policySuccessCount = 0;
        for (std::size_t run = 0; run < groups.size(); ++run)
        {
            const auto result = simulator.run(groups[run], policy, run);
            require(result.sharing.borrowRequests ==
                        result.sharing.successfulBorrows +
                            result.sharing.failedBorrows,
                    "Golden policy borrow request accounting changed");
            policySuccessCount += result.groupRepairSuccess ? 1 : 0;
            if (result.groupRepairSuccess)
            {
                require(result.usedRows + result.unusedPhysicalRows == 8 &&
                            result.usedColumns +
                                result.unusedPhysicalColumns == 8,
                        "Golden policy run violates physical conservation");
            }
        }
        return policySuccessCount;
    };
    dynamic_spare::SimulationConfig directional = config;
    directional.topology = dynamic_spare::SharingTopology::Directional;
    directional.sharedRows = 1;
    directional.sharedColumns = 1;
    directional.modifiers.localFirst = true;
    directional.modifiers.maximumGroupBorrowedSpares = 3;
    require(policySuccesses(directional) == 12,
            "Directional golden result changed from 12/32");

    dynamic_spare::SimulationConfig edge = directional;
    edge.topology = dynamic_spare::SharingTopology::PairwiseEdge;
    require(policySuccesses(edge) == 12,
            "Edge golden result changed from 12/32");

    dynamic_spare::SimulationConfig global = directional;
    global.topology = dynamic_spare::SharingTopology::GlobalPool;
    global.globalPool =
        dynamic_spare::GlobalPoolConfiguration{1, 1, 4, 4};
    require(policySuccesses(global) == 2,
            "Global-pool golden result changed from 2/32");
    const std::array<std::vector<std::size_t>, 4> firstValid{{
        {0, 3, 4}, {4}, {0, 3, 4}, {3, 4, 5}}};
    for (std::size_t subarray = 0; subarray < 4; ++subarray)
    {
        require(batch.runs.front().attemptsBySubarray[subarray]
                    .front().validCandidateIndices == firstValid[subarray],
                "Baseline runner changed golden valid candidate indices");
    }
    require(batch.runs.front().attemptsBySubarray[0]
                .front().hybridCamEntriesPeak.value() == 1 &&
                batch.runs.front().attemptsBySubarray[0]
                    .front().hybridCamEntriesActive == 0,
            "Hybrid-CAM peak instrumentation missed reclaimed entries");

    const std::filesystem::path firstDirectory =
        "/tmp/dynamic_spare_policy_csv_first";
    const std::filesystem::path secondDirectory =
        "/tmp/dynamic_spare_policy_csv_second";
    std::filesystem::create_directories(firstDirectory);
    std::filesystem::create_directories(secondDirectory);
    const std::vector<dynamic_spare::SimulationBatch> batches{batch};
    dynamic_spare::DynamicCsvReporter::writeAttempts(
        firstDirectory / "attempts.csv", batches);
    dynamic_spare::DynamicCsvReporter::writeRuns(
        firstDirectory / "runs.csv", batches);
    dynamic_spare::DynamicCsvReporter::writeSummary(
        firstDirectory / "summary.csv", batches);
    dynamic_spare::DynamicCsvReporter::writeAttempts(
        secondDirectory / "attempts.csv", batches);
    dynamic_spare::DynamicCsvReporter::writeRuns(
        secondDirectory / "runs.csv", batches);
    dynamic_spare::DynamicCsvReporter::writeSummary(
        secondDirectory / "summary.csv", batches);
    for (const std::string filename :
         {"attempts.csv", "runs.csv", "summary.csv"})
    {
        require(readFile(firstDirectory / filename) ==
                    readFile(secondDirectory / filename),
                "CSV output is not byte-deterministic: " + filename);
    }
    const std::string attemptsCsv = readFile(
        firstDirectory / "attempts.csv");
    require(static_cast<std::size_t>(std::count(
                attemptsCsv.begin(), attemptsCsv.end(), '\n')) == 129,
            "Baseline attempts.csv does not contain 128 attempt rows");
}

} // namespace

int main()
{
    try
    {
        verifyFaultGenerator();
        verifyDirectionalLedger();
        verifyGlobalAndEdgeLedgers();
        verifySimulatorMetrics();
        verifyGoldenRunnerAndCsv();
        std::cout << "Dynamic spare sharing policy tests passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
