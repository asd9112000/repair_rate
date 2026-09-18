#include "../inc/DynamicCsvReporter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dynamic_spare
{
namespace
{

struct PolicySemantics
{
    const char *solutionClass;
    const char *candidateContract;
    const char *priorityClass;
    const char *searchScope;
    bool backtracking;
};

PolicySemantics policySemantics(SolutionTakePolicy policy)
{
    switch (policy)
    {
        case SolutionTakePolicy::DirectionalV2Early:
            return {"LOCAL_FIRST", "DIRECTIONAL_V2", "LOCAL_FIRST",
                    "SEQUENTIAL_FIRST_LEGAL", false};
        case SolutionTakePolicy::GroupGreedyRtlCanonical:
            // This baseline commits one legal V2 slot per SA.  It never
            // evaluates feasibility of a future SA before that commitment.
            return {"EARLY", "DIRECTIONAL_V2", "RELEASE_AWARE",
                    "SEQUENTIAL_FIRST_LEGAL", false};
        case SolutionTakePolicy::DirectionalV2GroupGlobal:
            return {"GLOBAL", "DIRECTIONAL_V2", "JOINT_ORACLE",
                    "JOINT_COMPLETE_TUPLE_SEARCH", true};
        case SolutionTakePolicy::DirectionalV2GroupGlobalCanonical:
            return {"GLOBAL", "DIRECTIONAL_V2", "CANONICAL_R_L_RB_B",
                    "JOINT_COMPLETE_TUPLE_WITH_RELEASE_OBLIGATIONS", true};
        case SolutionTakePolicy::GroupGlobal:
            return {"GLOBAL", "GENERIC_RECAM", "JOINT_ORACLE",
                    "JOINT_GENERIC_GROUP_SEARCH", true};
        case SolutionTakePolicy::Early:
            return {"EARLY", "GENERIC_RECAM", "RELEASE_AWARE",
                    "SEQUENTIAL_RANKED_COMMIT", false};
        case SolutionTakePolicy::LocalFirst:
            return {"LOCAL_FIRST", "GENERIC_RECAM", "LOCAL_FIRST",
                    "SEQUENTIAL_FIRST_LEGAL", false};
        case SolutionTakePolicy::OneByFourTwoPairwiseEarlyV1:
        case SolutionTakePolicy::OneByFourSingleHopEarlyV1:
            return {"LOCAL_FIRST", "R1B_1X4", "LOCAL_FIRST",
                    "SEQUENTIAL_FIRST_LEGAL", false};
        case SolutionTakePolicy::OneByFourTwoPairwiseReleaseAwareEarlyV1:
        case SolutionTakePolicy::OneByFourSingleHopReleaseAwareEarlyV1:
            return {"EARLY", "R1B_1X4", "RELEASE_AWARE",
                    "SEQUENTIAL_RANKED_COMMIT", false};
        case SolutionTakePolicy::OneByFourTwoPairwisePairGlobalV1:
            return {"PAIR_GLOBAL", "R1B_1X4", "PAIR_JOINT_ORACLE",
                    "PAIR_COMPLETE_TUPLE_SEARCH", true};
        case SolutionTakePolicy::OneByFourSingleHopGlobalV1:
            return {"GLOBAL", "R1B_1X4", "JOINT_ORACLE",
                    "JOINT_COMPLETE_TUPLE_SEARCH", true};
        case SolutionTakePolicy::Legacy:
            return {"LOCAL", "LOCAL", "LOCAL_ONLY", "LOCAL", false};
        default:
            return {"OTHER", "POLICY_SPECIFIC", "UNSPECIFIED", "POLICY_SPECIFIC", false};
    }
}

const char *canonicalTopologyName(SharingTopology topology)
{
    switch (topology)
    {
        case SharingTopology::NoSharing: return "none";
        case SharingTopology::Directional: return "directional";
        case SharingTopology::GlobalPool: return "global";
        case SharingTopology::PairwiseEdge: return "edge";
        case SharingTopology::PairSharing: return "pair";
        case SharingTopology::NeighborSharing: return "neighbor";
    }
    return "unknown";
}

void fnv1aAppend(std::uint64_t &hash, std::uint64_t value)
{
    for (std::size_t byte = 0; byte < sizeof(value); ++byte)
    {
        hash ^= static_cast<unsigned char>(value >> (byte * 8));
        hash *= 1099511628211ULL;
    }
}

std::string pairedCorpusHash(
    const SimulationConfig &config,
    const std::vector<FaultGroup> &groups)
{
    std::uint64_t hash = 1469598103934665603ULL;
    fnv1aAppend(hash, config.randomSeed);
    fnv1aAppend(hash, config.faultCount);
    fnv1aAppend(hash, groups.size());
    for (const FaultGroup &group : groups)
    {
        for (const auto &subarray : group)
        {
            fnv1aAppend(hash, subarray.size());
            for (const Fault &fault : subarray)
            {
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.HBMID));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.ChannelID));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.BankID));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.SubarrayGroupID));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.SubarrayID));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.r));
                fnv1aAppend(hash, static_cast<std::uint32_t>(fault.c));
            }
        }
    }
    std::ostringstream output;
    output << std::hex << hash;
    return output.str();
}

std::ofstream openCsv(const std::filesystem::path &path, bool append)
{
    std::ofstream output(
        path,
        append ? std::ios::app : std::ios::trunc);
    if (!output)
    {
        throw std::runtime_error(
            "Unable to open CSV output: " + path.string());
    }
    output << std::setprecision(12);
    return output;
}

std::string formatIndices(const std::vector<std::size_t> &indices)
{
    if (indices.empty())
    {
        return "-";
    }
    std::ostringstream output;
    for (std::size_t index = 0; index < indices.size(); ++index)
    {
        if (index != 0)
        {
            output << '|';
        }
        output << indices[index];
    }
    return output.str();
}

std::string formatFaultGroup(const FaultGroup &group)
{
    std::ostringstream output;
    for (std::size_t subarray = 0; subarray < group.size(); ++subarray)
    {
        if (subarray != 0)
            output << '|';
        for (std::size_t index = 0; index < group[subarray].size(); ++index)
        {
            if (index != 0)
                output << ';';
            const Fault &fault = group[subarray][index];
            output << fault.HBMID << ':' << fault.ChannelID << ':'
                   << fault.BankID << ':' << fault.SubarrayGroupID << ':'
                   << fault.SubarrayID << ':' << fault.r << ':' << fault.c;
        }
    }
    return output.str();
}

std::string formatBitmap(const std::vector<bool> &bitmap)
{
    if (bitmap.empty())
        return "-";
    std::string result;
    result.reserve(bitmap.size());
    for (bool bit : bitmap)
        result.push_back(bit ? '1' : '0');
    return result;
}

std::string formatRemaining(
    const std::optional<RemainingSpareResources> &remaining)
{
    if (!remaining.has_value())
        return "-";
    return std::to_string(remaining->rows) + "|" +
        std::to_string(remaining->columns);
}

std::size_t totalAttempts(const GroupRepairResult &group)
{
    return std::accumulate(
        group.attemptsBySubarray.begin(),
        group.attemptsBySubarray.end(),
        std::size_t{0},
        [](std::size_t total,
           const std::vector<RepairAttemptResult> &attempts)
        {
            return total + attempts.size();
        });
}

const RepairAttemptResult &activeAttempt(
    const GroupRepairResult &group,
    std::size_t subarray)
{
    const auto &attempts = group.attemptsBySubarray[subarray];
    if (attempts.empty())
    {
        throw std::logic_error("Group result has no subarray attempt");
    }
    if (group.selectedAttemptIndices[subarray].has_value())
    {
        return attempts.at(*group.selectedAttemptIndices[subarray]);
    }
    return attempts.back();
}

struct RunHardwareMetrics
{
    std::size_t addressActive = 0;
    std::size_t addressPeak = 0;
    std::size_t addressProvisioned = 0;
    std::size_t hybridActive = 0;
    std::size_t hybridPeak = 0;
    std::size_t hybridProvisioned = 0;
    std::uint64_t hybridBits = 0;
    std::size_t bufferProvisioned = 0;
    std::size_t matrixActive = 0;
    std::size_t matrixPeak = 0;
    std::size_t matrixProvisioned = 0;
    std::uint64_t candidateSolutions = 0;
    std::uint64_t candidateSolutionsEvaluated = 0;
};

RunHardwareMetrics aggregateHardware(const GroupRepairResult &group)
{
    RunHardwareMetrics metrics;
    for (std::size_t subarray = 0;
         subarray < kSubarrayCount; ++subarray)
    {
        const RepairAttemptResult &active = activeAttempt(group, subarray);
        metrics.addressActive += active.addressCamEntriesActive;
        metrics.hybridActive += active.hybridCamEntriesActive;
        metrics.matrixActive += active.activeMatrixCells;

        std::size_t subarrayAddressPeak = 0;
        std::size_t subarrayAddressProvisioned = 0;
        std::size_t subarrayHybridPeak = 0;
        std::size_t subarrayHybridProvisioned = 0;
        std::size_t subarrayMatrixProvisioned = 0;
        std::size_t subarrayMatrixPeak = 0;
        std::uint64_t subarrayHybridBits = 0;
        std::size_t subarrayBufferProvisioned = 0;
        for (const RepairAttemptResult &attempt :
             group.attemptsBySubarray[subarray])
        {
            subarrayAddressPeak = std::max(
                subarrayAddressPeak,
                attempt.addressCamEntriesPeak.value_or(
                    attempt.addressCamEntriesActive));
            subarrayAddressProvisioned = std::max(
                subarrayAddressProvisioned,
                attempt.addressCamEntriesProvisioned);
            subarrayHybridPeak = std::max(
                subarrayHybridPeak,
                attempt.hybridCamEntriesPeak.value_or(
                    attempt.hybridCamEntriesActive));
            subarrayHybridProvisioned = std::max(
                subarrayHybridProvisioned,
                attempt.hybridCamEntriesProvisioned);
            subarrayMatrixProvisioned = std::max(
                subarrayMatrixProvisioned,
                attempt.provisionedMatrixCells);
            subarrayMatrixPeak = std::max(
                subarrayMatrixPeak,
                attempt.activeMatrixCells);
            subarrayHybridBits = std::max(
                subarrayHybridBits,
                attempt.hybridCamBitsProvisioned.value_or(0));
            subarrayBufferProvisioned = std::max(
                subarrayBufferProvisioned,
                attempt.bufferCamEntriesProvisioned);
            metrics.candidateSolutions += attempt.candidateSolutions;
            metrics.candidateSolutionsEvaluated +=
                attempt.candidateSolutionsEvaluated;
        }
        metrics.addressPeak += subarrayAddressPeak;
        metrics.addressProvisioned += subarrayAddressProvisioned;
        metrics.hybridPeak += subarrayHybridPeak;
        metrics.hybridProvisioned += subarrayHybridProvisioned;
        metrics.matrixProvisioned += subarrayMatrixProvisioned;
        metrics.matrixPeak += subarrayMatrixPeak;
        metrics.hybridBits += subarrayHybridBits;
        metrics.bufferProvisioned += subarrayBufferProvisioned;
    }
    return metrics;
}

std::uint64_t checkedMetricMultiply(
    std::uint64_t left,
    std::uint64_t right)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        throw std::overflow_error("Hardware-cost proxy multiplication overflow");
    }
    return left * right;
}

std::uint64_t checkedMetricAdd(
    std::uint64_t left,
    std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        throw std::overflow_error("Hardware-cost proxy addition overflow");
    }
    return left + right;
}

std::uint64_t hardwareCostProxyBits(
    const RunHardwareMetrics &metrics,
    const SimulationConfig &config)
{
    const std::uint64_t addressWidth =
        static_cast<std::uint64_t>(config.rowAddressWidthBits) +
        config.columnAddressWidthBits;
    const std::uint64_t bufferWidth = checkedMetricAdd(
        addressWidth, config.dataWidthBits);
    std::uint64_t total = checkedMetricMultiply(
        metrics.addressProvisioned, addressWidth);
    total = checkedMetricAdd(total, metrics.hybridBits);
    total = checkedMetricAdd(
        total,
        checkedMetricMultiply(metrics.bufferProvisioned, bufferWidth));
    return checkedMetricAdd(total, metrics.matrixProvisioned);
}

double divide(std::uint64_t numerator, std::uint64_t denominator)
{
    return denominator == 0
        ? 0.0
        : static_cast<double>(numerator) /
              static_cast<double>(denominator);
}

double percentile95(std::vector<std::uint64_t> values)
{
    if (values.empty())
    {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t rank = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(values.size())));
    return static_cast<double>(values[std::max<std::size_t>(1, rank) - 1]);
}

std::size_t baselineHybridEntries(const SimulationConfig &config)
{
    const std::size_t rows = static_cast<std::size_t>(config.spareRows);
    const std::size_t columns = static_cast<std::size_t>(config.spareColumns);
    return rows * (columns - 1) + columns * (rows - 1);
}

} // namespace

std::string DynamicCsvReporter::pairedCorpusId(
    const SimulationConfig &config,
    const std::vector<FaultGroup> &groups)
{
    return std::string(kPairedCorpusSchemaVersion) + "-" +
        pairedCorpusHash(config, groups);
}

void DynamicCsvReporter::writePairedCorpus(
    const std::filesystem::path &path,
    const SimulationConfig &config,
    const std::vector<FaultGroup> &groups,
    bool append)
{
    std::ofstream output = openCsv(path, append);
    if (!append)
    {
        output << "schema_version,corpus_id,corpus_hash,generator_version,seed,"
                  "simulation_level,topology,geometry,Rs,Cs,share_m,share_row,"
                  "share_col,fault_model,group_fault_count,group_id,device_id,"
                  "sa_local_faults\n";
    }
    const std::string corpusId = pairedCorpusId(config, groups);
    const std::string hash = pairedCorpusHash(config, groups);
    const int shareM = config.layout == GroupLayout::Line1x4
        ? config.sharedRows : std::max(config.sharedRows, config.sharedColumns);
    for (std::size_t groupId = 0; groupId < groups.size(); ++groupId)
    {
        output << kPairedCorpusSchemaVersion << ',' << corpusId << ',' << hash
               << ",dynamic_fault_generator_v1," << config.randomSeed
               << ",group," << toString(config.topology) << ','
               << toString(config.layout) << ',' << config.spareRows << ','
               << config.spareColumns << ',' << shareM << ','
               << config.sharedRows << ',' << config.sharedColumns << ','
               << toString(config.faultCountModel) << ',' << config.faultCount
               << ',' << groupId << ",-," << formatFaultGroup(groups[groupId])
               << '\n';
    }
}

void DynamicCsvReporter::writePairedPolicyResults(
    const std::filesystem::path &path,
    const SimulationConfig &config,
    const std::vector<FaultGroup> &groups,
    const std::vector<GroupRepairResult> &runs,
    bool append)
{
    if (groups.size() != runs.size())
        throw std::invalid_argument("Paired corpus/results group count differs");
    std::ofstream output = openCsv(path, append);
    if (!append)
    {
        output << "schema_version,corpus_id,corpus_hash,group_id,canonical_policy_id,implementation_policy_id,policy_id,sharing_policy,topology,layout,share_row,share_col,N,RS,CS,F_GROUP,seed,paper_canonical,legacy_alias_of,"
                  "solution_policy,config_contract_version,solution_class,"
                  "candidate_contract,priority_class,search_scope,backtracking,"
                  "tier2_cam_required,tier2_unique_tag_count,tier2_tag_contract_version,simulator_revision,"
                  "repair_success,selected_config_A,selected_config_B,"
                  "selected_config_C,selected_config_D,selected_pattern_A,"
                  "selected_pattern_B,selected_pattern_C,selected_pattern_D,"
                  "borrowed_rows,borrowed_columns,remaining_rows,remaining_columns,"
                  "private_rows_per_SA,private_columns_per_SA,shareable_rows_per_SA,"
                  "total_rows_group,total_columns_group,total_physical_group,"
                  "local_row_used,local_column_used,own_shareable_row_used,"
                  "borrowed_shareable_row_used,remaining_shareable_rows,"
                  "cam_accounting_status,address_cam_used_entries,"
                  "address_cam_capacity_entries,address_cam_entry_bits,"
                  "address_cam_capacity_bits,hybrid_cam_used_entries,"
                  "hybrid_cam_capacity_entries,hybrid_cam_entry_bits,"
                  "hybrid_cam_capacity_bits,total_cam_capacity_bits,"
                  "global_candidate_count_A,global_candidate_count_B,"
                  "global_candidate_count_C,global_candidate_count_D,"
                  "global_raw_cartesian_product_size,global_search_nodes_visited,"
                  "global_partial_assignments_pruned,global_complete_assignments_checked,"
                  "global_legal_complete_assignments,global_first_feasible_node_index,"
                  "global_runtime_us,global_left_donor_transfers,"
                  "global_right_donor_transfers,global_middle_donor_choice_branches,"
                  "global_stopped_at_first_legal,global_exhaustive_enumeration,"
                  "global_termination_reason\n";
    }
    const std::string corpusId = pairedCorpusId(config, groups);
    const std::string corpusHash = pairedCorpusHash(config, groups);
    for (std::size_t groupId = 0; groupId < runs.size(); ++groupId)
    {
        const GroupRepairResult &run = runs[groupId];
        const bool lineLayout = config.layout == GroupLayout::Line1x4;
        const RunHardwareMetrics hardware = aggregateHardware(run);
        const PolicySemantics semantics = policySemantics(run.solutionTakePolicy);
        output << kPairedCorpusSchemaVersion << ',' << corpusId << ',' << corpusHash
               << ',' << groupId << ',' << config.canonicalPolicyId << ','
               << toString(run.solutionTakePolicy) << ','
               << toString(run.solutionTakePolicy) << ','
               << toString(config.topology) << ','
               << canonicalTopologyName(config.topology) << ','
               << toString(config.layout) << ',' << config.sharedRows << ','
               << config.sharedColumns << ',' << config.spareRows << ','
               << config.spareRows << ',' << config.spareColumns << ','
               << config.faultCount << ',' << config.randomSeed << ','
               << (config.paperCanonical ? "true" : "false") << ','
               << (config.legacyAliasOf.empty() ? "-" : config.legacyAliasOf) << ','
               << toString(run.solutionTakePolicy) << ','
               << toString(run.configContractVersion) << ','
               << semantics.solutionClass << ',' << semantics.candidateContract
               << ',' << semantics.priorityClass << ',' << semantics.searchScope << ','
               << (semantics.backtracking ? "true" : "false")
               << ',' << (run.tier2CamRequired ? 1 : 0)
               << ',' << run.tier2UniqueTagCount
               << ',' << run.tier2TagContractVersion
               << ",cpp_dynamic_repair_v1,"
               << (run.groupRepairSuccess ? 1 : 0);
        for (const auto &configId : run.selectedConfigIds)
            output << ',' << (configId.has_value() ? std::to_string(*configId) : "-");
        for (const auto &patternId : run.selectedPatternIds)
            output << ',' << (patternId.has_value() ? std::to_string(*patternId) : "-");
        output << ',' << run.sharing.borrowedRows << ','
               << run.sharing.borrowedColumns << ','
               << run.unusedPhysicalRows << ',' << run.unusedPhysicalColumns
               << ',' << run.privateRowCountPerSubarray
               << ',' << run.privateColumnCountPerSubarray
               << ',' << run.shareableRowCountPerSubarray
               << ',' << run.totalRowSpareLinesGroup
               << ',' << run.totalColumnSpareLinesGroup
               << ',' << run.totalPhysicalSpareLinesGroup
               << ',' << run.localRowUsed << ',' << run.localColumnUsed
               << ',' << run.ownShareableRowUsed
               << ',' << run.borrowedShareableRowUsed
               << ',' << run.remainingShareableRows;
        if (lineLayout)
        {
            // R1B established 1x4 repair semantics, not a topology-calibrated
            // CAM capacity or occupancy formula.
            output << ",NOT_PROVEN,NA,NA,NA,NA,NA,NA,NA,NA,NA";
        }
        else
        {
            const std::uint64_t addressEntryBits =
                static_cast<std::uint64_t>(config.rowAddressWidthBits) +
                config.columnAddressWidthBits;
            const std::uint64_t addressCapacityBits =
                checkedMetricMultiply(hardware.addressProvisioned, addressEntryBits);
            const std::uint64_t hybridEntryBits =
                config.hybridCamEntryWidthBits.value_or(0);
            const std::uint64_t hybridCapacityBits =
                checkedMetricMultiply(hardware.hybridProvisioned, hybridEntryBits);
            output << ",GENERIC_CPP_CAPACITY,"
                   << hardware.addressActive << ','
                   << hardware.addressProvisioned << ','
                   << addressEntryBits << ',' << addressCapacityBits << ','
                   << hardware.hybridActive << ','
                   << hardware.hybridProvisioned << ','
                   << hybridEntryBits << ',' << hybridCapacityBits << ','
                   << checkedMetricAdd(addressCapacityBits, hybridCapacityBits);
        }
        const GlobalSearchMetrics *search = run.globalSearchMetrics.has_value()
            ? &*run.globalSearchMetrics : nullptr;
        for (std::size_t subarray = 0; subarray < kSubarrayCount; ++subarray)
            output << ',' << (search ? search->candidateCounts[subarray] : 0);
        output << ',' << (search ? search->rawCartesianProductSize : 0)
               << ',' << (search ? search->searchNodesVisited : 0)
               << ',' << (search ? search->partialAssignmentsPruned : 0)
               << ',' << (search ? search->completeAssignmentsChecked : 0)
               << ',' << (search ? search->legalCompleteAssignments : 0)
               << ',' << (search && search->firstFeasibleNodeIndex.has_value()
                               ? std::to_string(*search->firstFeasibleNodeIndex)
                               : "-1")
               << ',' << (search ? search->runtimeMicroseconds : 0)
               << ',' << (search ? search->leftDonorTransfers : 0)
               << ',' << (search ? search->rightDonorTransfers : 0)
               << ',' << (search ? search->middleSaDonorChoiceBranches : 0)
               << ',' << (search && search->stoppedAtFirstLegal ? 1 : 0)
               << ',' << (search && search->exhaustiveEnumeration ? 1 : 0)
               << ',' << (search && !search->terminationReason.empty()
                               ? search->terminationReason : "-")
               << '\n';
    }
}

void DynamicCsvReporter::writeAttempts(
    const std::filesystem::path &path,
    const std::vector<SimulationBatch> &batches,
    bool append)
{
    std::ofstream output = openCsv(path, append);
    if (!append)
    {
        output
        << "seed,run_index,fault_model,layout,policy,subarray_id,attempt_index,stage,"
           "available_rows,available_columns,fault_count,pivot_fault_count,"
           "nonpivot_fault_count,buffered_pivot_fault_count,"
           "overflow_pivot_fault_count,is_repairable,cam_storage_overflow,"
           "repair_success,used_rows,used_columns,unused_accessible_rows,"
           "unused_accessible_columns,address_CAM_active,address_CAM_peak,"
           "address_CAM_provisioned,hybrid_CAM_active,hybrid_CAM_peak,"
           "hybrid_CAM_provisioned,buffer_CAM_active,buffer_CAM_provisioned,"
           "hybrid_CAM_entry_width_bits,hybrid_CAM_bits_active,"
           "hybrid_CAM_bits_provisioned,matrix_dimension,matrix_cells_active,"
           "matrix_cells_provisioned,candidate_solutions,"
           "candidate_solutions_evaluated,failed_candidates,"
           "successful_candidate_index,valid_candidate_indices,"
           "analysis_attempts,fault_information_insert_cycles,"
           "fault_information_lookup_cycles,fault_information_read_cycles,"
           "matrix_generation_cycles,solution_generation_cycles,"
           "solution_evaluation_cycles,sharing_allocation_cycles,"
           "total_analysis_cycles,solution_take_policy,valid_solution_bitmap,"
           "compressed_state_bits\n";
    }

    for (const SimulationBatch &batch : batches)
    {
        for (const GroupRepairResult &group : batch.runs)
        {
            for (std::size_t subarray = 0;
                 subarray < kSubarrayCount; ++subarray)
            {
                for (const RepairAttemptResult &attempt :
                     group.attemptsBySubarray[subarray])
                {
                    output
                        << group.seed << ',' << group.runIndex << ','
                        << toString(batch.config.faultCountModel) << ','
                        << toString(batch.config.layout) << ','
                        << toString(batch.config.topology) << ','
                        << attempt.subarrayId << ',' << attempt.attemptIndex
                        << ',' << attempt.stage << ',' << attempt.availableRows
                        << ',' << attempt.availableColumns << ','
                        << attempt.faultCount << ','
                        << attempt.pivotFaultCount << ','
                        << attempt.nonpivotFaultCount << ','
                        << attempt.bufferedPivotFaultCount << ','
                        << attempt.overflowPivotFaultCount << ','
                        << (attempt.isRepairable ? 1 : 0) << ','
                        << (attempt.camStorageOverflow ? 1 : 0) << ','
                        << (attempt.repairSuccess ? 1 : 0) << ','
                        << attempt.usedRows << ',' << attempt.usedColumns << ','
                        << attempt.unusedAccessibleRows << ','
                        << attempt.unusedAccessibleColumns << ','
                        << attempt.addressCamEntriesActive << ','
                        << attempt.addressCamEntriesPeak.value_or(0) << ','
                        << attempt.addressCamEntriesProvisioned << ','
                        << attempt.hybridCamEntriesActive << ','
                        << attempt.hybridCamEntriesPeak.value_or(0) << ','
                        << attempt.hybridCamEntriesProvisioned << ','
                        << attempt.bufferCamEntriesActive << ','
                        << attempt.bufferCamEntriesProvisioned << ','
                        << attempt.hybridCamEntryWidthBits.value_or(0) << ','
                        << attempt.hybridCamBitsActive.value_or(0) << ','
                        << attempt.hybridCamBitsProvisioned.value_or(0) << ','
                        << attempt.matrixDimension << ','
                        << attempt.activeMatrixCells << ','
                        << attempt.provisionedMatrixCells << ','
                        << attempt.candidateSolutions << ','
                        << attempt.candidateSolutionsEvaluated << ','
                        << attempt.failedCandidates << ','
                        << (attempt.successfulCandidateIndex.has_value()
                                ? std::to_string(
                                      *attempt.successfulCandidateIndex)
                                : "-1")
                        << ',' << formatIndices(attempt.validCandidateIndices)
                        << ',' << attempt.analysisAttempts << ','
                        << attempt.latency.faultInformationInsertCycles << ','
                        << attempt.latency.faultInformationLookupCycles << ','
                        << attempt.latency.faultInformationReadCycles << ','
                        << attempt.latency.matrixGenerationCycles << ','
                        << attempt.latency.solutionGenerationCycles << ','
                        << attempt.latency.solutionEvaluationCycles << ','
                        << attempt.latency.sharingAllocationCycles << ','
                        << attempt.latency.totalCycles() << ','
                        << toString(batch.config.solutionTakePolicy) << ','
                        << (attempt.tileSolutionState.has_value()
                                ? formatBitmap(attempt.tileSolutionState
                                      ->validSolutionBitmap)
                                : "-")
                        << ',' << (attempt.tileSolutionState.has_value()
                                ? attempt.tileSolutionState
                                      ->compressedStorageBits
                                : 0)
                        << '\n';
                }
            }
        }
    }
}

void DynamicCsvReporter::writeRuns(
    const std::filesystem::path &path,
    const std::vector<SimulationBatch> &batches,
    bool append)
{
    std::ofstream output = openCsv(path, append);
    if (!append)
    {
        output
        << "seed,run_index,fault_model,fault_spatial_model,layout,policy,storage_mode,"
           "data_width_bits,row_address_width_bits,column_address_width_bits,"
           "hybrid_CAM_entry_width_bits,Rs,Cs,shared_rows,shared_columns,"
           "global_local_rows,global_local_columns,global_rows,global_columns,"
           "local_first,minimum_row_reserve,"
           "minimum_column_reserve,single_dimension,max_group_borrows,"
           "fault_A,fault_B,fault_C,fault_D,repair_A,repair_B,repair_C,"
           "repair_D,selected_attempt_A,selected_attempt_B,selected_attempt_C,"
           "selected_attempt_D,selected_candidate_A,selected_candidate_B,"
           "selected_candidate_C,selected_candidate_D,"
           "local_group_repair_success,baseline_group_repair_success,"
           "group_repair_success,repair_after_1_borrow,"
           "repair_after_2_borrows,repair_after_3_borrows,sharing_gain,"
           "used_rows,used_columns,unused_rows,unused_columns,row_utilization,"
           "column_utilization,total_spare_utilization,borrowed_rows,"
           "borrowed_columns,lent_rows,lent_columns,borrow_requests,"
           "successful_borrows,failed_borrows,donor_starvation_count,"
           "global_rows_used,global_columns_used,remaining_global_rows,"
           "remaining_global_columns,repair_success_due_to_sharing,"
           "repair_failure_even_after_sharing,address_CAM_active,"
           "address_CAM_peak,address_CAM_provisioned,hybrid_CAM_active,"
           "hybrid_CAM_peak,hybrid_CAM_provisioned,hybrid_CAM_bits,"
           "matrix_cells_active,matrix_cells_provisioned,"
           "hardware_cost_proxy_bits,candidate_solutions,"
           "candidate_solutions_evaluated,analysis_attempts,"
           "extra_analysis_attempts,fault_information_insert_cycles,"
           "fault_information_lookup_cycles,fault_information_read_cycles,"
           "matrix_generation_cycles,solution_generation_cycles,"
           "solution_evaluation_cycles,sharing_allocation_cycles,"
           "analysis_cycles,solution_take_policy,valid_solution_bitmap_A,"
           "valid_solution_bitmap_B,valid_solution_bitmap_C,"
           "valid_solution_bitmap_D,feasible_combination_count,"
           "solution_selection_work,compressed_state_bits,early_success,"
           "group_compressed_success,greedy_loss,remaining_after_A,"
           "remaining_after_B,remaining_after_C,remaining_after_D,"
           "solution_selection_failure_reason,tier2_cam_required,"
           "tier2_unique_tag_count,tier2_tag_contract_version\n";
    }

    for (const SimulationBatch &batch : batches)
    {
        const std::size_t physicalRows =
            kSubarrayCount * static_cast<std::size_t>(batch.config.spareRows);
        const std::size_t physicalColumns =
            kSubarrayCount * static_cast<std::size_t>(batch.config.spareColumns);
        for (const GroupRepairResult &group : batch.runs)
        {
            const RunHardwareMetrics hardware = aggregateHardware(group);
            const bool localGroupSuccess = std::all_of(
                group.localRepairSuccess.begin(),
                group.localRepairSuccess.end(),
                [](bool success) { return success; });
            output
                << group.seed << ',' << group.runIndex << ','
                << toString(batch.config.faultCountModel) << ','
                << toString(batch.config.faultSpatialModel) << ','
                << toString(batch.config.layout) << ','
                << toString(batch.config.topology) << ','
                << toString(batch.config.storageMode) << ','
                << batch.config.dataWidthBits << ','
                << batch.config.rowAddressWidthBits << ','
                << batch.config.columnAddressWidthBits << ','
                << batch.config.hybridCamEntryWidthBits.value_or(0) << ','
                << batch.config.spareRows << ','
                << batch.config.spareColumns << ','
                << batch.config.sharedRows << ','
                << batch.config.sharedColumns << ','
                << (batch.config.globalPool.has_value()
                        ? batch.config.globalPool->localRowsPerSubarray : 0)
                << ','
                << (batch.config.globalPool.has_value()
                        ? batch.config.globalPool->localColumnsPerSubarray : 0)
                << ','
                << (batch.config.globalPool.has_value()
                        ? batch.config.globalPool->globalRows : 0)
                << ','
                << (batch.config.globalPool.has_value()
                        ? batch.config.globalPool->globalColumns : 0)
                << ','
                << (batch.config.modifiers.localFirst ? 1 : 0) << ','
                << batch.config.modifiers.minimumRowReserve << ','
                << batch.config.modifiers.minimumColumnReserve << ','
                << (batch.config.modifiers.singleDimensionBorrowing ? 1 : 0)
                << ','
                << batch.config.modifiers.maximumGroupBorrowedSpares;
            for (std::size_t count : group.faultCounts)
            {
                output << ',' << count;
            }
            for (bool success : group.repairSuccess)
            {
                output << ',' << (success ? 1 : 0);
            }
            for (const auto &selected : group.selectedAttemptIndices)
            {
                output << ',' << (selected.has_value()
                    ? std::to_string(*selected) : "-1");
            }
            for (const auto &selected : group.selectedCandidateIndices)
            {
                output << ',' << (selected.has_value()
                    ? std::to_string(*selected) : "-1");
            }
            output
                << ',' << (localGroupSuccess ? 1 : 0)
                << ',' << (group.baselineGroupRepairSuccess ? 1 : 0)
                << ',' << (group.groupRepairSuccess ? 1 : 0)
                << ',' << (group.repairedAfterBorrowCount(1) ? 1 : 0)
                << ',' << (group.repairedAfterBorrowCount(2) ? 1 : 0)
                << ',' << (group.repairedAfterBorrowCount(3) ? 1 : 0)
                << ',' << group.sharingGain
                << ',' << group.usedRows << ',' << group.usedColumns
                << ',' << group.unusedPhysicalRows
                << ',' << group.unusedPhysicalColumns
                << ',' << divide(group.usedRows, physicalRows)
                << ',' << divide(group.usedColumns, physicalColumns)
                << ',' << divide(
                    group.usedRows + group.usedColumns,
                    physicalRows + physicalColumns)
                << ',' << group.sharing.borrowedRows
                << ',' << group.sharing.borrowedColumns
                << ',' << group.sharing.lentRows
                << ',' << group.sharing.lentColumns
                << ',' << group.sharing.borrowRequests
                << ',' << group.sharing.successfulBorrows
                << ',' << group.sharing.failedBorrows
                << ',' << group.sharing.donorStarvationCount
                << ',' << group.sharing.globalRowsUsed
                << ',' << group.sharing.globalColumnsUsed
                << ',' << group.sharing.remainingGlobalRows
                << ',' << group.sharing.remainingGlobalColumns
                << ',' << (group.sharing.repairSuccessDueToSharing ? 1 : 0)
                << ',' << (group.sharing.repairFailureEvenAfterSharing ? 1 : 0)
                << ',' << hardware.addressActive
                << ',' << hardware.addressPeak
                << ',' << hardware.addressProvisioned
                << ',' << hardware.hybridActive
                << ',' << hardware.hybridPeak
                << ',' << hardware.hybridProvisioned
                << ',' << hardware.hybridBits
                << ',' << hardware.matrixActive
                << ',' << hardware.matrixProvisioned
                << ',' << hardwareCostProxyBits(hardware, batch.config)
                << ',' << hardware.candidateSolutions
                << ',' << hardware.candidateSolutionsEvaluated
                << ',' << totalAttempts(group)
                << ',' << group.extraAnalysisAttempts
                << ',' << group.latency.faultInformationInsertCycles
                << ',' << group.latency.faultInformationLookupCycles
                << ',' << group.latency.faultInformationReadCycles
                << ',' << group.latency.matrixGenerationCycles
                << ',' << group.latency.solutionGenerationCycles
                << ',' << group.latency.solutionEvaluationCycles
                << ',' << group.latency.sharingAllocationCycles
                << ',' << group.latency.totalCycles()
                << ',' << toString(group.solutionTakePolicy);
            for (const auto &bitmap : group.validSolutionBitmaps)
                output << ',' << formatBitmap(bitmap);
            output
                << ',' << group.feasibleCombinationCount
                << ',' << group.solutionSelectionWork
                << ',' << group.compressedStateBits
                << ',' << (group.earlySuccess.has_value()
                        ? (*group.earlySuccess ? "1" : "0") : "-")
                << ',' << (group.groupCompressedSuccess.has_value()
                        ? (*group.groupCompressedSuccess ? "1" : "0") : "-")
                << ',' << (group.greedyLoss ? 1 : 0);
            for (const auto &remaining : group.remainingResourcesAfterTile)
                output << ',' << formatRemaining(remaining);
            output << ',' << (group.solutionSelectionFailureReason.empty()
                    ? "-" : group.solutionSelectionFailureReason)
                   << ',' << (group.tier2CamRequired ? 1 : 0)
                   << ',' << group.tier2UniqueTagCount
                   << ',' << group.tier2TagContractVersion
                   << '\n';
        }
    }
}

void DynamicCsvReporter::writeSummary(
    const std::filesystem::path &path,
    const std::vector<SimulationBatch> &batches,
    bool append)
{
    std::ofstream output = openCsv(path, append);
    if (!append)
    {
        output
        << "seed,fault_count,fault_model,fault_spatial_model,layout,policy,storage_mode,"
           "data_width_bits,row_address_width_bits,column_address_width_bits,"
           "hybrid_CAM_entry_width_bits,Rs,Cs,shared_rows,shared_columns,"
           "global_local_rows,global_local_columns,global_rows,global_columns,"
           "local_first,minimum_row_reserve,"
           "minimum_column_reserve,single_dimension,max_group_borrows,runs,"
           "repair_rate,baseline_repair_rate,sharing_gain,"
           "average_spare_utilization,average_borrow_count,p95_borrow_count,"
           "average_CAM_requirement,peak_CAM_requirement,"
           "average_matrix_size,peak_matrix_size,average_candidate_count,"
           "average_analysis_attempts,average_analysis_cycles,"
           "p95_analysis_cycles,average_hardware_cost_proxy_bits,"
           "peak_hardware_cost_proxy_bits,repair_gain_per_extra_CAM_entry,"
           "repair_gain_per_matrix_cell,repair_gain_per_analysis_cycle,"
           "repair_gain_per_borrowed_spare,local_subarray_repairs,"
           "groups_requiring_borrow,total_borrowed_rows,"
           "average_borrowed_rows,maximum_borrowed_rows,solution_take_policy,"
           "greedy_loss_count,greedy_loss_rate,"
           "average_feasible_combinations,maximum_feasible_combinations,"
           "average_group_selector_candidates_checked,"
           "average_compressed_state_bits,tier2_cam_required_groups,"
           "average_tier2_unique_tag_count,maximum_tier2_unique_tag_count,"
           "tier2_tag_contract_version\n";
    }

    for (const SimulationBatch &batch : batches)
    {
        const std::uint64_t runCount = batch.runs.size();
        std::uint64_t successes = 0;
        std::uint64_t baselineSuccesses = 0;
        std::uint64_t usedSpares = 0;
        std::uint64_t borrowedSpares = 0;
        std::uint64_t borrowedRows = 0;
        std::uint64_t maximumBorrowedRows = 0;
        std::uint64_t greedyLossCount = 0;
        std::uint64_t feasibleCombinations = 0;
        std::uint64_t maximumFeasibleCombinations = 0;
        std::uint64_t selectorCandidatesChecked = 0;
        std::uint64_t compressedStateBits = 0;
        std::uint64_t localSubarrayRepairs = 0;
        std::uint64_t groupsRequiringBorrow = 0;
        std::uint64_t camRequirement = 0;
        std::uint64_t peakCamRequirement = 0;
        std::uint64_t matrixSize = 0;
        std::uint64_t peakMatrixSize = 0;
        std::uint64_t candidates = 0;
        std::uint64_t attempts = 0;
        std::uint64_t cycles = 0;
        std::uint64_t baselineCycles = 0;
        std::uint64_t provisionedCam = 0;
        std::uint64_t provisionedMatrix = 0;
        std::uint64_t hardwareCost = 0;
        std::uint64_t peakHardwareCost = 0;
        std::uint64_t tier2CamRequiredGroups = 0;
        std::uint64_t tier2UniqueTagCount = 0;
        std::uint64_t maximumTier2UniqueTagCount = 0;
        std::string tier2TagContractVersion = "-";
        std::vector<std::uint64_t> borrowSamples;
        std::vector<std::uint64_t> cycleSamples;

        for (const GroupRepairResult &group : batch.runs)
        {
            const RunHardwareMetrics hardware = aggregateHardware(group);
            successes += group.groupRepairSuccess ? 1 : 0;
            baselineSuccesses += group.baselineGroupRepairSuccess ? 1 : 0;
            usedSpares += group.usedRows + group.usedColumns;
            const std::uint64_t borrowed =
                group.sharing.borrowedRows + group.sharing.borrowedColumns;
            borrowedSpares += borrowed;
            borrowedRows += group.sharing.borrowedRows;
            maximumBorrowedRows = std::max<std::uint64_t>(
                maximumBorrowedRows, group.sharing.borrowedRows);
            greedyLossCount += group.greedyLoss ? 1 : 0;
            feasibleCombinations += group.feasibleCombinationCount;
            maximumFeasibleCombinations = std::max(
                maximumFeasibleCombinations,
                group.feasibleCombinationCount);
            selectorCandidatesChecked += group.solutionSelectionWork;
            compressedStateBits += group.compressedStateBits;
            localSubarrayRepairs += static_cast<std::uint64_t>(
                std::count(
                    group.localRepairSuccess.begin(),
                    group.localRepairSuccess.end(), true));
            groupsRequiringBorrow +=
                group.groupRepairSuccess && borrowed != 0 ? 1 : 0;
            borrowSamples.push_back(borrowed);
            const std::uint64_t activeCam =
                hardware.addressActive + hardware.hybridActive;
            camRequirement += activeCam;
            peakCamRequirement = std::max<std::uint64_t>(
                peakCamRequirement,
                hardware.addressPeak + hardware.hybridPeak);
            matrixSize += hardware.matrixActive;
            peakMatrixSize = std::max<std::uint64_t>(
                peakMatrixSize, hardware.matrixPeak);
            candidates += hardware.candidateSolutionsEvaluated;
            attempts += totalAttempts(group);
            const std::uint64_t runCycles = group.latency.totalCycles();
            cycles += runCycles;
            baselineCycles += group.baselineAnalysisCycles;
            cycleSamples.push_back(runCycles);
            provisionedCam +=
                hardware.addressProvisioned + hardware.hybridProvisioned;
            provisionedMatrix += hardware.matrixProvisioned;
            const std::uint64_t runHardwareCost = hardwareCostProxyBits(
                hardware, batch.config);
            hardwareCost += runHardwareCost;
            peakHardwareCost = std::max(
                peakHardwareCost, runHardwareCost);
            tier2CamRequiredGroups += group.tier2CamRequired ? 1 : 0;
            tier2UniqueTagCount += group.tier2UniqueTagCount;
            maximumTier2UniqueTagCount = std::max(
                maximumTier2UniqueTagCount,
                static_cast<std::uint64_t>(group.tier2UniqueTagCount));
            if (group.tier2TagContractVersion != "-")
                tier2TagContractVersion = group.tier2TagContractVersion;
        }

        const std::uint64_t physicalSparesPerRun =
            kSubarrayCount * static_cast<std::uint64_t>(
                batch.config.spareRows + batch.config.spareColumns);
        const double repairRate = divide(successes, runCount);
        const double baselineRate = divide(baselineSuccesses, runCount);
        const double repairGain = repairRate - baselineRate;
        const std::uint64_t baselineCamPerRun =
            kSubarrayCount * static_cast<std::uint64_t>(
                batch.config.spareRows + batch.config.spareColumns +
                baselineHybridEntries(batch.config));
        const std::uint64_t baselineMatrixPerRun =
            kSubarrayCount * static_cast<std::uint64_t>(
                batch.config.spareRows + batch.config.spareColumns) *
            static_cast<std::uint64_t>(
                batch.config.spareRows + batch.config.spareColumns);
        const double extraCam = runCount == 0
            ? 0.0
            : std::max(
                  0.0,
                  divide(provisionedCam, runCount) -
                      static_cast<double>(baselineCamPerRun));
        const double extraMatrix = runCount == 0
            ? 0.0
            : std::max(
                  0.0,
                  divide(provisionedMatrix, runCount) -
                      static_cast<double>(baselineMatrixPerRun));
        const double extraCycles = runCount == 0
            ? 0.0
            : std::max(
                  0.0,
                  divide(cycles, runCount) -
                      divide(baselineCycles, runCount));
        const double averageBorrow = divide(borrowedSpares, runCount);

        output
            << batch.config.randomSeed << ','
            << batch.config.faultCount << ','
            << toString(batch.config.faultCountModel) << ','
            << toString(batch.config.faultSpatialModel) << ','
            << toString(batch.config.layout) << ','
            << toString(batch.config.topology) << ','
            << toString(batch.config.storageMode) << ','
            << batch.config.dataWidthBits << ','
            << batch.config.rowAddressWidthBits << ','
            << batch.config.columnAddressWidthBits << ','
            << batch.config.hybridCamEntryWidthBits.value_or(0) << ','
            << batch.config.spareRows << ',' << batch.config.spareColumns << ','
            << batch.config.sharedRows << ','
            << batch.config.sharedColumns << ','
            << (batch.config.globalPool.has_value()
                    ? batch.config.globalPool->localRowsPerSubarray : 0)
            << ','
            << (batch.config.globalPool.has_value()
                    ? batch.config.globalPool->localColumnsPerSubarray : 0)
            << ','
            << (batch.config.globalPool.has_value()
                    ? batch.config.globalPool->globalRows : 0)
            << ','
            << (batch.config.globalPool.has_value()
                    ? batch.config.globalPool->globalColumns : 0)
            << ','
            << (batch.config.modifiers.localFirst ? 1 : 0) << ','
            << batch.config.modifiers.minimumRowReserve << ','
            << batch.config.modifiers.minimumColumnReserve << ','
            << (batch.config.modifiers.singleDimensionBorrowing ? 1 : 0)
            << ',' << batch.config.modifiers.maximumGroupBorrowedSpares
            << ',' << runCount
            << ',' << repairRate
            << ',' << baselineRate
            << ',' << repairGain
            << ',' << divide(usedSpares, runCount * physicalSparesPerRun)
            << ',' << averageBorrow
            << ',' << percentile95(borrowSamples)
            << ',' << divide(camRequirement, runCount)
            << ',' << peakCamRequirement
            << ',' << divide(matrixSize, runCount)
            << ',' << peakMatrixSize
            << ',' << divide(candidates, runCount)
            << ',' << divide(attempts, runCount)
            << ',' << divide(cycles, runCount)
            << ',' << percentile95(cycleSamples)
            << ',' << divide(hardwareCost, runCount)
            << ',' << peakHardwareCost
            << ',' << (extraCam == 0.0 ? 0.0 : repairGain / extraCam)
            << ',' << (extraMatrix == 0.0 ? 0.0 : repairGain / extraMatrix)
            << ',' << (extraCycles == 0.0 ? 0.0 : repairGain / extraCycles)
            << ',' << (averageBorrow == 0.0
                            ? 0.0
                            : repairGain / averageBorrow)
            << ',' << localSubarrayRepairs
            << ',' << groupsRequiringBorrow
            << ',' << borrowedRows
            << ',' << divide(borrowedRows, runCount)
            << ',' << maximumBorrowedRows
            << ',' << toString(batch.config.solutionTakePolicy)
            << ',' << greedyLossCount
            << ',' << divide(greedyLossCount, runCount)
            << ',' << divide(feasibleCombinations, runCount)
            << ',' << maximumFeasibleCombinations
            << ',' << divide(selectorCandidatesChecked, runCount)
            << ',' << divide(compressedStateBits, runCount)
            << ',' << tier2CamRequiredGroups
            << ',' << divide(tier2UniqueTagCount, runCount)
            << ',' << maximumTier2UniqueTagCount
            << ',' << tier2TagContractVersion
            << '\n';
    }
}

} // namespace dynamic_spare
