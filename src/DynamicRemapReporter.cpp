#include "../inc/DynamicRemapReporter.hpp"

#include <fstream>
#include <optional>
#include <stdexcept>
#include <tuple>

namespace dynamic_spare
{
namespace
{

using GroupIdentity = std::tuple<int, int, int, int>;

std::ofstream openOutput(const std::filesystem::path &path)
{
    std::ofstream output(path);
    if (!output.is_open())
    {
        throw std::runtime_error(
            "Unable to open remap output: " + path.string());
    }
    return output;
}

std::optional<GroupIdentity> identityOf(const FaultGroup &group)
{
    for (const auto &faults : group)
    {
        if (!faults.empty())
        {
            const Fault &fault = faults.front();
            return GroupIdentity{
                fault.HBMID,
                fault.ChannelID,
                fault.BankID,
                fault.SubarrayGroupID};
        }
    }
    return std::nullopt;
}

void writeLineMapping(std::ostream &output, const RepairLineMapping &mapping)
{
    output
        << "MAP "
        << mapping.HBMID << ' '
        << mapping.ChannelID << ' '
        << mapping.BankID << ' '
        << mapping.SubarrayGroupID << ' '
        << mapping.SubarrayID << ' '
        << mapping.sourceRow << ' '
        << mapping.sourceColumn << ' '
        << (mapping.dimension == SpareDimension::Row ? 'R' : 'C') << ' '
        << mapping.replacementAddress << ' '
        << mapping.latency << '\n';
}

void writeBufferMapping(
    std::ostream &output,
    const BufferRepairMapping &mapping)
{
    output
        << "BUFFMAP "
        << mapping.HBMID << ' '
        << mapping.ChannelID << ' '
        << mapping.BankID << ' '
        << mapping.SubarrayGroupID << ' '
        << mapping.SubarrayID << ' '
        << mapping.sourceRow << ' '
        << mapping.sourceColumn << ' '
        << mapping.latency << '\n';
}

void writeFailure(
    std::ostream &output,
    const GroupIdentity &identity)
{
    output
        << "#FAILED SUBARRAY GROUP "
        << std::get<0>(identity) << ' '
        << std::get<1>(identity) << ' '
        << std::get<2>(identity) << ' '
        << std::get<3>(identity) << '\n';
}

} // namespace

RemapWriteSummary DynamicRemapReporter::write(
    const std::filesystem::path &fullPath,
    const std::filesystem::path &simplifiedPath,
    const SimulationConfig &config,
    const std::vector<FaultGroup> &faultGroups,
    const std::vector<GroupRepairResult> &results)
{
    if (faultGroups.size() != results.size())
    {
        throw std::invalid_argument(
            "Remap reporter requires one result per fault group");
    }

    std::ofstream full = openOutput(fullPath);
    std::ofstream simplified = openOutput(simplifiedPath);
    const bool bufferDisabled =
        !config.usePaperCamReuseCapacity && config.bufferCamEntries == 0;
    full
        << "# REMAP_TABLE_LOG 2\n"
        << "# CAM_REUSE_MODE "
        << (config.usePaperCamReuseCapacity
                ? "PAPER_ADDITIONAL_PIVOT"
                : (bufferDisabled
                       ? "BUFFER_DISABLED"
                       : "FIXED_ADDITIONAL_PIVOT_BUFFER"))
        << '\n'
        << "# BUFFER_ENTRIES_PER_PE ";
    if (config.usePaperCamReuseCapacity)
    {
        full << "Rs+Cs\n";
    }
    else
    {
        full << config.bufferCamEntries << '\n';
    }
    full
        << "# HYBRID_OVERFLOW_TO_BUFFER_EXTENSION 0\n"
        << "# GROUP_LAYOUT " << toString(config.layout) << "\n"
        << "# DYNAMIC_POLICY " << toString(config.topology) << "\n"
        << "# OPTION <pattern_id> <option_id> <config_index>\n"
        << "# PE <pe_id> <spare_rows> <spare_cols> <solution_index>\n"
        << "# MAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> "
           "<SubarrayID> <r> <c> <R|C> <new_address> <latency>\n"
        << "# BUFFMAP <HBMID> <ChannelID> <BankID> <SubarrayGroupID> "
           "<SubarrayID> <r> <c> <latency>\n"
        << "#FAILED SUBARRAY GROUP <HBMID> <ChannelID> <BankID> "
           "<SubarrayGroupID>\n";

    RemapWriteSummary summary;
    for (std::size_t pattern = 0; pattern < results.size(); ++pattern)
    {
        const GroupRepairResult &group = results[pattern];
        if (!group.groupRepairSuccess)
        {
            const auto identity = identityOf(faultGroups[pattern]);
            if (!identity.has_value())
            {
                throw std::logic_error(
                    "Cannot identify an empty failed fault group for remap "
                    "output");
            }
            writeFailure(full, *identity);
            writeFailure(simplified, *identity);
            ++summary.failedGroups;
            continue;
        }

        // DynamicSpareSharing emits one canonical, group-feasible option per
        // successful pattern.  option_id/config_index are therefore both 0.
        full << "OPTION " << pattern << " 0 0\n";
        for (std::size_t subarray = 0;
             subarray < kSubarrayCount; ++subarray)
        {
            if (!group.selectedAttemptIndices[subarray].has_value() ||
                !group.selectedCandidateIndices[subarray].has_value() ||
                !group.selectedCandidateOptions[subarray].has_value())
            {
                throw std::logic_error(
                    "Successful dynamic group is missing its retained remap "
                    "selection");
            }
            const std::size_t attemptIndex =
                *group.selectedAttemptIndices[subarray];
            if (attemptIndex >= group.attemptsBySubarray[subarray].size())
            {
                throw std::logic_error(
                    "Selected dynamic remap attempt index is out of range");
            }
            const RepairAttemptResult &attempt =
                group.attemptsBySubarray[subarray][attemptIndex];
            const CandidateRepairOption &candidate =
                *group.selectedCandidateOptions[subarray];
            if (candidate.candidateIndex !=
                *group.selectedCandidateIndices[subarray])
            {
                throw std::logic_error(
                    "Retained dynamic remap candidate does not match the "
                    "group selection");
            }
            if (candidate.bufferCamRemapCount !=
                candidate.bufferMappings.size())
            {
                throw std::logic_error(
                    "Retained BUFFMAP entries do not match the selected "
                    "candidate's buffer count");
            }

            full
                << "PE " << subarray << ' '
                << attempt.availableRows << ' '
                << attempt.availableColumns << ' '
                << candidate.candidateIndex << '\n';
            for (const RepairLineMapping &mapping : candidate.mappings)
            {
                writeLineMapping(full, mapping);
                writeLineMapping(simplified, mapping);
                ++summary.lineMappings;
            }
            for (const BufferRepairMapping &mapping :
                 candidate.bufferMappings)
            {
                writeBufferMapping(full, mapping);
                writeBufferMapping(simplified, mapping);
                ++summary.bufferMappings;
            }
            full << "END_PE\n";
        }
        full << "END_OPTION\n";
        ++summary.successfulGroups;
    }

    if (!full || !simplified)
    {
        throw std::runtime_error("Failed while writing dynamic remap output");
    }
    return summary;
}

} // namespace dynamic_spare
