#include "../inc/DynamicRemapReporter.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <stdexcept>
#include <tuple>

namespace dynamic_spare
{
namespace
{

using GroupIdentity = std::tuple<int, int, int, int>;
using RuntimeEntryKey = std::tuple<std::size_t, int, int, int, int, int, int, int>;

struct RuntimeEntryProfile
{
    std::size_t slot = 0;
    std::uint64_t searchRounds = 0;
    std::uint64_t readHitCycles = 0;
    std::uint64_t writeHitCycles = 0;
};

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

RuntimeEntryKey runtimeKey(
    std::size_t pattern,
    const BufferRepairMapping &mapping)
{
    return {pattern, mapping.HBMID, mapping.ChannelID, mapping.BankID,
            mapping.SubarrayGroupID, mapping.SubarrayID, mapping.sourceRow,
            mapping.sourceColumn};
}

std::map<RuntimeEntryKey, RuntimeEntryProfile> assignRuntimeProfiles(
    const std::vector<GroupRepairResult> &results,
    const RuntimeRepairLatencyConfig &config)
{
    std::map<RuntimeEntryKey, RuntimeEntryProfile> profiles;
    for (std::size_t pattern = 0; pattern < results.size(); ++pattern)
    {
        const GroupRepairResult &group = results[pattern];
        if (!group.groupRepairSuccess)
            continue;
        for (const auto &candidate : group.selectedCandidateOptions)
        {
            if (!candidate.has_value())
                continue;
            for (const BufferRepairMapping &mapping : candidate->bufferMappings)
                profiles.emplace(runtimeKey(pattern, mapping), RuntimeEntryProfile{});
        }
    }

    std::size_t nextSlot = 0;
    std::size_t currentPattern = static_cast<std::size_t>(-1);
    std::size_t patternEntries = 0;
    for (auto &entry : profiles)
    {
        const std::size_t pattern = std::get<0>(entry.first);
        if (pattern != currentPattern)
        {
            currentPattern = pattern;
            nextSlot = 0;
            patternEntries = 0;
            for (const auto &candidate : profiles)
                if (std::get<0>(candidate.first) == pattern)
                    ++patternEntries;
        }
        RuntimeEntryProfile &profile = entry.second;
        profile.slot = nextSlot++;
        if (config.storage == RuntimeRepairStorage::Cam)
        {
            profile.searchRounds = 1;
            profile.readHitCycles = config.camReadHitCycles;
            profile.writeHitCycles = config.camWriteHitCycles;
            continue;
        }
        const std::size_t parallelism =
            config.storage == RuntimeRepairStorage::SramWide
                ? std::max<std::size_t>(1, patternEntries)
                : config.sramParallelism;
        profile.searchRounds = profile.slot / parallelism + 1;
        const std::uint64_t searchCycles = profile.searchRounds *
            (config.registeredSearch ? 2U : 1U);
        profile.readHitCycles = searchCycles + config.sramDataReadCycles +
            config.sramMuxCycles;
        profile.writeHitCycles = searchCycles + config.sramDataWriteCycles;
    }
    return profiles;
}

void writeRuntimeManifest(
    const std::filesystem::path &path,
    const std::map<RuntimeEntryKey, RuntimeEntryProfile> &profiles,
    const RuntimeRepairLatencyConfig &config)
{
    std::ofstream output = openOutput(path);
    output << "# RUNTIME_REPAIR_TABLE_V1\n"
           << "# Scope: independent 4-SA group per pattern; slots reset per pattern.\n"
           << "# BUFFMAP latency is runtime_read_hit_latency_cycles.\n"
           << "pattern_id,HBMID,ChannelID,BankID,SubarrayGroupID,SubarrayID,row,column,"
              "backend,slot,search_rounds,read_hit_latency_cycles,"
              "write_hit_latency_cycles\n";
    for (const auto &entry : profiles)
    {
        const auto &[pattern, hbm, channel, bank, group, subarray, row, column] =
            entry.first;
        const RuntimeEntryProfile &profile = entry.second;
        output << pattern << ',' << hbm << ',' << channel << ',' << bank << ','
               << group << ',' << subarray << ',' << row << ',' << column << ','
               << toString(config.storage) << ',' << profile.slot << ','
               << profile.searchRounds << ',' << profile.readHitCycles << ','
               << profile.writeHitCycles << '\n';
    }
    if (!output)
        throw std::runtime_error("Failed while writing runtime repair manifest: " +
                                 path.string());
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

const char *toString(RuntimeRepairStorage storage) noexcept
{
    switch (storage)
    {
        case RuntimeRepairStorage::Cam: return "cam";
        case RuntimeRepairStorage::SramSerial: return "sram_serial";
        case RuntimeRepairStorage::SramChunked: return "sram_chunked";
        case RuntimeRepairStorage::SramWide: return "sram_wide";
    }
    return "unknown";
}

void RuntimeRepairLatencyConfig::validate() const
{
    if (storage == RuntimeRepairStorage::SramChunked && sramParallelism == 0)
        throw std::invalid_argument("SRAM chunked runtime parallelism must be positive");
}

RemapWriteSummary DynamicRemapReporter::write(
    const std::filesystem::path &fullPath,
    const std::filesystem::path &simplifiedPath,
    const SimulationConfig &config,
    const std::vector<FaultGroup> &faultGroups,
    const std::vector<GroupRepairResult> &results,
    const RuntimeRepairLatencyConfig &runtimeConfig)
{
    if (faultGroups.size() != results.size())
    {
        throw std::invalid_argument(
            "Remap reporter requires one result per fault group");
    }

    runtimeConfig.validate();
    const auto runtimeProfiles = assignRuntimeProfiles(results, runtimeConfig);
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
        << "# BUFFMAP_LATENCY_SEMANTICS RUNTIME_READ_HIT_RESPONSE_CYCLES\n"
        << "# RUNTIME_REPAIR_BACKEND " << toString(runtimeConfig.storage) << "\n"
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
                BufferRepairMapping timed = mapping;
                const auto profile = runtimeProfiles.find(runtimeKey(pattern, mapping));
                if (profile == runtimeProfiles.end())
                    throw std::logic_error("Selected BUFFMAP has no runtime profile");
                timed.latency = static_cast<int>(profile->second.readHitCycles);
                writeBufferMapping(full, timed);
                writeBufferMapping(simplified, timed);
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
    writeRuntimeManifest(fullPath.parent_path() / "RuntimeRepairTable.csv",
                         runtimeProfiles, runtimeConfig);
    summary.runtimeRepairEntries = runtimeProfiles.size();
    return summary;
}

} // namespace dynamic_spare
