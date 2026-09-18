#include "DynamicRepairSimulator.hpp"
#include "PhysicalResourceLedger.hpp"
#include "SimplifiedFaultLoader.hpp"
#include "SimulationConfig.hpp"
#include "V2GroupNoScratchPolicy.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace
{
using namespace dynamic_spare;

SimulationConfig configFor(int n, int faults, SolutionTakePolicy policy)
{
    SimulationConfig config;
    config.spareRows = n;
    config.spareColumns = n;
    config.sharedRows = 1;
    config.sharedColumns = 1;
    config.layout = GroupLayout::Grid2x2;
    config.topology = SharingTopology::Directional;
    config.solutionTakePolicy = policy;
    config.usePaperCamReuseCapacity = true;
    config.faultCountModel = FaultCountModel::MultinomialUniform;
    config.faultCount = faults;
    config.hybridCamEntryWidthBits = 20;
    return config;
}

bool releases(V2GroupAction action)
{
    return action == V2GroupAction::ReleaseOnly ||
        action == V2GroupAction::ReleaseAndBorrow;
}

char role(std::size_t subarray)
{
    return std::array<char, 4>{{'A', 'B', 'C', 'D'}}.at(subarray);
}

std::string resourceName(const GroupRepairResult::SelectedBorrowTransfer &transfer)
{
    return std::string(1, role(static_cast<std::size_t>(transfer.donorSubarray))) +
        (transfer.dimension == SpareDimension::Row ? "_ROW" : "_COL");
}

template <typename T>
std::string optionalValue(const std::optional<T> &value)
{
    return value.has_value() ? std::to_string(*value) : "-";
}

bool sameTuple(const GroupRepairResult &left, const GroupRepairResult &right)
{
    return left.selectedConfigIds == right.selectedConfigIds &&
        left.selectedPatternIds == right.selectedPatternIds;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: canonical_global_corpus_audit CORPUS_ROOT OUTPUT_DIR\n";
        return 2;
    }
    const std::filesystem::path root = argv[1];
    const std::filesystem::path output = argv[2];
    std::filesystem::create_directories(output);
    std::ofstream events(output / "current_global_future_borrows.csv");
    std::ofstream comparison(output / "old_vs_canonical_global.csv");
    events << "topology,N,F_GROUP,group_id,borrower,donor,resource,borrower_action,donor_action,borrower_pattern_id,donor_pattern_id,matched\n";
    comparison << "N,F_GROUP,group_id,old_success,canonical_success,tuple_diff,early_success,canonical_actions,canonical_configs,canonical_patterns\n";

    std::uint64_t groupsChecked = 0;
    std::uint64_t futureBorrows = 0;
    std::uint64_t matched = 0;
    std::uint64_t unmatched = 0;
    std::uint64_t repairabilityDifferences = 0;
    std::uint64_t selectedTupleDifferences = 0;
    std::uint64_t earlyPassCanonicalFail = 0;
    std::uint64_t canonicalInvariantViolations = 0;
    DynamicRepairSimulator simulator;
    for (int n : {2, 3})
    {
        for (int faults : {8, 12, 16, 20, 24, 28, 32})
        {
            const std::string point = "n" + std::to_string(n) + "_f" +
                std::to_string(faults);
            const auto corpus = loadSimplifiedFaultGroups(
                root / "corpus" / point / "corpus.simplified.faults",
                configFor(n, faults, SolutionTakePolicy::DirectionalV2GroupGlobal));
            if (corpus.size() != 1000)
                throw std::runtime_error(point + " is not the frozen 1k corpus");
            for (std::size_t groupId = 0; groupId < corpus.size(); ++groupId)
            {
                const GroupRepairResult old = simulator.run(
                    corpus[groupId], configFor(n, faults,
                        SolutionTakePolicy::DirectionalV2GroupGlobal), groupId, false);
                const GroupRepairResult canonical = simulator.run(
                    corpus[groupId], configFor(n, faults,
                        SolutionTakePolicy::DirectionalV2GroupGlobalCanonical), groupId, false);
                const GroupRepairResult early = simulator.run(
                    corpus[groupId], configFor(n, faults,
                        SolutionTakePolicy::GroupGreedyRtlCanonical), groupId, false);
                ++groupsChecked;
                repairabilityDifferences +=
                    old.groupRepairSuccess != canonical.groupRepairSuccess;
                const bool tupleDiff = old.groupRepairSuccess &&
                    canonical.groupRepairSuccess && !sameTuple(old, canonical);
                selectedTupleDifferences += tupleDiff;
                earlyPassCanonicalFail += early.groupRepairSuccess &&
                    !canonical.groupRepairSuccess;

                std::set<std::size_t> physicalIds;
                for (const auto &transfer : canonical.selectedBorrowTransfers)
                {
                    if (!physicalIds.insert(transfer.physicalLineId).second)
                        ++canonicalInvariantViolations;
                    if (transfer.donorSubarray < 0)
                        ++canonicalInvariantViolations;
                    else
                    {
                        PhysicalResourceLedger ledger(configFor(
                            n, faults,
                            SolutionTakePolicy::DirectionalV2GroupGlobalCanonical));
                        if (!ledger.canBorrow(transfer.donorSubarray,
                                              transfer.borrowerSubarray,
                                              transfer.dimension))
                            ++canonicalInvariantViolations;
                        if (static_cast<std::size_t>(transfer.donorSubarray) >
                            transfer.borrowerSubarray)
                        {
                            const auto action = canonical.selectedV2Actions.at(
                                static_cast<std::size_t>(transfer.donorSubarray));
                            if (!action.has_value() || !releases(*action))
                                ++canonicalInvariantViolations;
                        }
                    }
                }

                for (const auto &transfer : old.selectedBorrowTransfers)
                {
                    if (transfer.donorSubarray < 0 ||
                        static_cast<std::size_t>(transfer.donorSubarray) <=
                            transfer.borrowerSubarray)
                        continue;
                    ++futureBorrows;
                    const std::size_t donor = static_cast<std::size_t>(
                        transfer.donorSubarray);
                    const bool isMatched = old.selectedV2Actions[donor].has_value() &&
                        releases(*old.selectedV2Actions[donor]);
                    matched += isMatched;
                    unmatched += !isMatched;
                    events << "directional," << n << ',' << faults << ',' << groupId
                           << ',' << role(transfer.borrowerSubarray) << ',' << role(donor)
                           << ',' << resourceName(transfer) << ','
                           << toString(*old.selectedV2Actions[transfer.borrowerSubarray])
                           << ',' << toString(*old.selectedV2Actions[donor]) << ','
                           << optionalValue(old.selectedPatternIds[transfer.borrowerSubarray])
                           << ',' << optionalValue(old.selectedPatternIds[donor]) << ','
                           << (isMatched ? 1 : 0) << '\n';
                }
                comparison << n << ',' << faults << ',' << groupId << ','
                           << old.groupRepairSuccess << ',' << canonical.groupRepairSuccess
                           << ',' << tupleDiff << ',' << early.groupRepairSuccess << ',';
                for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
                    comparison << (sa ? ":" : "")
                               << (canonical.selectedV2Actions[sa].has_value()
                                       ? toString(*canonical.selectedV2Actions[sa]) : "-");
                comparison << ',';
                for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
                    comparison << (sa ? ":" : "")
                               << optionalValue(canonical.selectedConfigIds[sa]);
                comparison << ',';
                for (std::size_t sa = 0; sa < kSubarrayCount; ++sa)
                    comparison << (sa ? ":" : "")
                               << optionalValue(canonical.selectedPatternIds[sa]);
                comparison << '\n';
            }
            std::cout << "AUDITED_POINT=" << point << " GROUPS=" << corpus.size() << '\n';
        }
    }
    std::cout << "GROUPS_CHECKED=" << groupsChecked << '\n'
              << "CURRENT_GLOBAL_FUTURE_BORROWS=" << futureBorrows << '\n'
              << "CURRENT_GLOBAL_MATCHED_FUTURE_BORROWS=" << matched << '\n'
              << "CURRENT_GLOBAL_UNMATCHED_FUTURE_BORROWS=" << unmatched << '\n'
              << "OLD_CANONICAL_GLOBAL_REPAIRABILITY_DIFF=" << repairabilityDifferences << '\n'
              << "OLD_CANONICAL_GLOBAL_SELECTED_TUPLE_DIFF=" << selectedTupleDifferences << '\n'
              << "EARLY_PASS_CANONICAL_GLOBAL_FAIL=" << earlyPassCanonicalFail << '\n'
              << "CANONICAL_INVARIANT_VIOLATIONS=" << canonicalInvariantViolations << '\n';
    return canonicalInvariantViolations == 0 && earlyPassCanonicalFail == 0 ? 0 : 1;
}
