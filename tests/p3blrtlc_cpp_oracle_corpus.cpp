#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "RepairAttemptSolver.hpp"
#include "SimulationConfig.hpp"
#include "SolGenerator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::size_t kCases = 1000;
constexpr std::uint32_t kSeed = 20260921;
constexpr std::size_t kSubarrays = 4;
constexpr std::size_t kAttempts = 3;
constexpr std::size_t kPatterns = 15;

struct AttemptRecord {
    std::array<bool, kPatterns> valid{};
    std::array<bool, 6> rowPresent{};
    std::array<bool, 6> columnPresent{};
};
using Case = std::array<std::array<AttemptRecord, kAttempts>, kSubarrays>;

struct CandidateStatistics {
    std::vector<std::size_t> rawPerGroup;
    std::vector<std::size_t> canonicalPerGroup;
    std::size_t rawTotal = 0;
    std::size_t canonicalTotal = 0;
};

std::array<double, 5> summarize(const std::vector<std::size_t> &values) {
    std::vector<std::size_t> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t last = sorted.size() - 1;
    const std::size_t total = std::accumulate(sorted.begin(), sorted.end(), std::size_t{0});
    return {{
        static_cast<double>(total) / sorted.size(),
        static_cast<double>(sorted[last * 50 / 100]),
        static_cast<double>(sorted[last * 95 / 100]),
        static_cast<double>(sorted[last * 99 / 100]),
        static_cast<double>(sorted.back())
    }};
}

std::size_t choose(std::size_t n, std::size_t r) {
    r = std::min(r, n - r);
    std::size_t result = 1;
    for (std::size_t index = 1; index <= r; ++index)
        result = result * (n - r + index) / index;
    return result;
}

std::size_t attemptCount(std::size_t subarray) {
    return (subarray == 0 || subarray == 3) ? 2 : 3;
}

std::size_t candidateCount(std::size_t attempt) {
    return choose(4 + attempt, 2 + attempt);
}

dynamic_spare::CandidateRepairOption decodeOption(
    const AttemptRecord &record,
    int rows,
    int columns,
    std::size_t pattern) {
    SolGenerator generator(rows, columns);
    const solVector &orientation = generator.allSolVectorsType.at(pattern);
    dynamic_spare::CandidateRepairOption option;
    option.candidateIndex = pattern;
    for (std::size_t index = 0; index < orientation.size(); ++index) {
        if (orientation[index] && record.columnPresent[index])
            ++option.usedColumns;
        if (!orientation[index] && record.rowPresent[index])
            ++option.usedRows;
    }
    return option;
}

class CorpusSolver final : public dynamic_spare::RepairAttemptSolver {
public:
    explicit CorpusSolver(const std::array<Case, kCases> &corpus)
        : corpus_(corpus) {}

    dynamic_spare::RepairAttemptResult solve(
        const std::vector<Fault> &,
        const dynamic_spare::RECAMSolverRequest &request) const override {
        using namespace dynamic_spare;
        RepairAttemptResult result;
        result.runIndex = request.runIndex;
        result.attemptIndex = request.attemptIndex;
        result.stage = request.stage;
        result.subarrayId = request.subarrayId;
        result.availableRows = request.availableRows;
        result.availableColumns = request.availableColumns;
        result.provisionedRows = request.provisionedRows;
        result.provisionedColumns = request.provisionedColumns;
        const std::size_t subarray = static_cast<std::size_t>(request.subarrayId);
        const std::size_t attempt = request.attemptIndex;
        const std::size_t dimension = static_cast<std::size_t>(
            request.availableRows + request.availableColumns);
        result.candidateSolutions = choose(
            dimension, static_cast<std::size_t>(request.availableRows));
        result.candidateSolutionsEvaluated = result.candidateSolutions;

        const AttemptRecord &record = corpus_.at(request.runIndex).at(subarray).at(attempt);
        TileSolutionState state;
        state.subarrayId = request.subarrayId;
        state.spareRows = request.availableRows;
        state.spareColumns = request.availableColumns;
        state.matrixRowAddresses.resize(dimension);
        state.matrixColumnAddresses.resize(dimension);
        state.validSolutionBitmap.assign(result.candidateSolutions, false);
        state.compressedStorageBits = 1;
        for (std::size_t index = 0; index < dimension; ++index) {
            if (record.rowPresent[index])
                state.matrixRowAddresses[index] = MatrixRepairAddress{
                    0, 0, 0, 0, request.subarrayId, int(index), int(index)};
            if (record.columnPresent[index])
                state.matrixColumnAddresses[index] = MatrixRepairAddress{
                    0, 0, 0, 0, request.subarrayId, int(index), int(index)};
        }
        for (std::size_t pattern = 0; pattern < result.candidateSolutions; ++pattern) {
            if (!record.valid[pattern])
                continue;
            state.validSolutionBitmap[pattern] = true;
            result.validCandidateIndices.push_back(pattern);
            result.validCandidateOptions.push_back(decodeOption(
                record, request.availableRows, request.availableColumns, pattern));
        }
        result.repairSuccess = !result.validCandidateOptions.empty();
        result.isRepairable = result.repairSuccess;
        result.failedCandidates = result.candidateSolutions - result.validCandidateOptions.size();
        result.tileSolutionState = std::move(state);
        return result;
    }

private:
    const std::array<Case, kCases> &corpus_;
};

std::array<Case, kCases> makeCorpus() {
    std::array<Case, kCases> corpus{};
    std::mt19937 generator(kSeed);
    for (Case &record : corpus) {
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray) {
            for (std::size_t attempt = 0; attempt < attemptCount(subarray); ++attempt) {
                AttemptRecord &entry = record[subarray][attempt];
                const std::size_t dimension = 4 + attempt;
                for (std::size_t index = 0; index < dimension; ++index) {
                    entry.rowPresent[index] = (generator() % 100) < 65;
                    entry.columnPresent[index] = true;
                }
                for (std::size_t pattern = 0; pattern < candidateCount(attempt); ++pattern)
                    entry.valid[pattern] = (generator() % 100) < 14;
            }
        }
    }
    return corpus;
}

void writeCandidateTable(
    std::ofstream &output,
    const dynamic_spare::GroupRepairResult &group) {
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray) {
        for (std::size_t attempt = 0; attempt < kAttempts; ++attempt) {
            std::array<dynamic_spare::CandidateRepairOption, kPatterns> options{};
            std::array<bool, kPatterns> valid{};
            if (attempt < group.attemptsBySubarray[subarray].size()) {
                for (const auto &option : group.attemptsBySubarray[subarray][attempt].validCandidateOptions) {
                    valid.at(option.candidateIndex) = true;
                    options.at(option.candidateIndex) = option;
                }
            }
            for (std::size_t pattern = 0; pattern < kPatterns; ++pattern) {
                output << ' ' << valid[pattern] << ' ' << options[pattern].usedRows
                       << ' ' << options[pattern].usedColumns;
            }
        }
    }
}

void writeCase(
    std::ofstream &output,
    const dynamic_spare::GroupRepairResult &group,
    const dynamic_spare::PhysicalResourceLedger &ledger) {
    std::array<dynamic_spare::SpareDemand, 4> demands{};
    if (group.groupRepairSuccess) {
        for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray) {
            demands[subarray] = {group.selectedCandidateOptions[subarray]->usedRows,
                                  group.selectedCandidateOptions[subarray]->usedColumns};
        }
    }
    const auto allocation = ledger.allocateSequential(
        demands, group.groupRepairSuccess ? 4 : 0);
    output << (group.groupRepairSuccess ? 1 : 0) << ' '
           << allocation.transfers.size() << ' ' << allocation.usedRows;
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray) {
        if (group.groupRepairSuccess) {
            output << ' ' << *group.selectedAttemptIndices[subarray]
                   << ' ' << (*group.selectedCandidateIndices[subarray] + 1)
                   << ' ' << group.selectedCandidateOptions[subarray]->usedRows
                   << ' ' << group.selectedCandidateOptions[subarray]->usedColumns;
        } else {
            output << " 3 0 0 0";
        }
    }
    std::array<unsigned, 4> donors{{15, 15, 15, 15}};
    std::array<unsigned, 4> donorCount{};
    for (const auto &transfer : allocation.transfers) {
        if (transfer.dimension != dynamic_spare::SpareDimension::Row)
            continue;
        const std::size_t borrower = transfer.borrowerSubarray;
        donors[borrower] &= ~(3U << (donorCount[borrower] * 2));
        donors[borrower] |= (unsigned(transfer.donorSubarray) & 3U) <<
            (donorCount[borrower] * 2);
        ++donorCount[borrower];
    }
    for (unsigned donor : donors)
        output << ' ' << donor;
    for (const auto &line : allocation.lines) {
        if (line.dimension == dynamic_spare::SpareDimension::Row)
            output << ' ' << (line.assignedSubarray ? int(*line.assignedSubarray) : 4);
    }
    writeCandidateTable(output, group);
    output << '\n';
}

void recordCandidateStatistics(
    CandidateStatistics &statistics,
    const dynamic_spare::GroupRepairResult &group) {
    std::size_t raw = 0;
    std::size_t canonical = 0;
    for (std::size_t subarray = 0; subarray < kSubarrays; ++subarray) {
        std::set<std::pair<std::size_t, std::size_t>> demands;
        for (const auto &attempt : group.attemptsBySubarray[subarray]) {
            for (const auto &option : attempt.validCandidateOptions) {
                ++raw;
                demands.emplace(option.usedRows, option.usedColumns);
            }
        }
        canonical += demands.size();
    }
    statistics.rawPerGroup.push_back(raw);
    statistics.canonicalPerGroup.push_back(canonical);
    statistics.rawTotal += raw;
    statistics.canonicalTotal += canonical;
}
} // namespace

int main() {
    const auto corpus = makeCorpus();
    dynamic_spare::SimulationConfig config;
    config.layout = dynamic_spare::GroupLayout::Line1x4;
    config.topology = dynamic_spare::SharingTopology::NeighborSharing;
    config.sharedRows = 1;
    config.sharedColumns = 0;
    config.modifiers.maximumGroupBorrowedSpares = 3;
    config.solutionTakePolicy = dynamic_spare::SolutionTakePolicy::OneByFourSingleHopGlobalV1;
    const dynamic_spare::PhysicalResourceLedger ledger(config);
    dynamic_spare::DynamicRepairSimulator simulator(
        std::make_shared<CorpusSolver>(corpus));
    auto earlyConfig = config;
    earlyConfig.solutionTakePolicy =
        dynamic_spare::SolutionTakePolicy::OneByFourSingleHopEarlyV1;
    bool earlyGlobalDistinction = false;
    bool earlyFailsGlobal = false;
    CandidateStatistics candidateStatistics;
    std::ofstream output("tmp/p3blrtlc_cpp_rtl_oracle.txt");
    if (!output)
        throw std::runtime_error("cannot write GLOBAL oracle corpus");
    output << kCases << ' ' << kSeed << '\n';
    for (std::size_t index = 0; index < kCases; ++index) {
        const auto global = simulator.run({}, config, index, true);
        const auto early = simulator.run({}, earlyConfig, index, true);
        earlyGlobalDistinction = earlyGlobalDistinction ||
            global.groupRepairSuccess != early.groupRepairSuccess;
        earlyFailsGlobal = earlyFailsGlobal ||
            (!early.groupRepairSuccess && global.groupRepairSuccess);
        recordCandidateStatistics(candidateStatistics, global);
        writeCase(output, global, ledger);
    }
    if (!earlyGlobalDistinction || !earlyFailsGlobal)
        throw std::runtime_error("GLOBAL corpus lacks an EARLY-fail/GLOBAL-pass witness");
    const auto rawStatistics = summarize(candidateStatistics.rawPerGroup);
    const auto canonicalStatistics = summarize(candidateStatistics.canonicalPerGroup);
    const double reduction = 1.0 - static_cast<double>(candidateStatistics.canonicalTotal) /
        static_cast<double>(candidateStatistics.rawTotal);
    std::cout << "CPP_RTL_GLOBAL_SHARED_CORPUS=YES\n"
              << "CPP_ORACLE_RANDOM_CASES=" << kCases << '\n'
              << "CPP_ORACLE_SEED=" << kSeed << '\n'
              << "EARLY_GLOBAL_DISTINCTION_TEST=PASS\n"
              << std::fixed << std::setprecision(3)
              << "SYN_D_RAW_CANDIDATE_STATS_MEAN_MEDIAN_P95_P99_MAX="
              << rawStatistics[0] << ',' << rawStatistics[1] << ','
              << rawStatistics[2] << ',' << rawStatistics[3] << ','
              << rawStatistics[4] << '\n'
              << "SYN_D_OPT1_CANDIDATE_STATS_MEAN_MEDIAN_P95_P99_MAX="
              << canonicalStatistics[0] << ',' << canonicalStatistics[1] << ','
              << canonicalStatistics[2] << ',' << canonicalStatistics[3] << ','
              << canonicalStatistics[4] << '\n'
              << "SYN_D_OPT1_CANDIDATE_REDUCTION_RATIO=" << reduction << '\n';
}
